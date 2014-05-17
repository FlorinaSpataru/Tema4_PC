// Spataru Florina Gabriela 323 CA // florina.spataru@gmail.com
// Tema 4 PC // Aplicatie client pentru interogari DNS

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "dns_message.h"

using namespace std;

#define DNS "127.0.0.1"
#define DNSPORT 53
#define BUFLEN 1024
#define NAMESZ 256

#define WAIT 2 // 2 seconds waiting time

char domain_name[BUFLEN];

// Convert type from int to string
void type_to_string(int type, char* string){
   switch(type){
      case A:{
         strcpy(string, "A");
         break;
      }
      case NS:{
         strcpy(string, "NS");
         break;
      }
      case CNAME:{
         strcpy(string, "CNAME");
         break;
      }
      case MX:{
         strcpy(string, "MX");
         break;
      }
      case SOA:{
         strcpy(string, "SOA");
         break;
      }
      case TXT:{
         strcpy(string, "TXT");
         break;
      }
      default:{
         strcpy(string, "");
         break;
      }
   }
}

// Reverse conversion - string to int
int string_to_type(char *type){
   if(strcmp(type, "A") == 0)
      return A;
   else if(strcmp(type, "NS") == 0)
      return NS;
   else if(strcmp(type, "CNAME") == 0)
      return CNAME;
   else if(strcmp(type, "MX") == 0)
      return MX;
   else if(strcmp(type, "SOA") == 0)
      return SOA;
   else if(strcmp(type, "TXT") == 0)
      return TXT;
   else
      return -1;
}

// Convert from www.google.com format to 3www6google3com
void host_to_dns(char *dns, char *host){
   int count = 0;
   char *token = strtok((char*)host,".");

   while (token != NULL){
      dns[count] = strlen(token);
      count++;
      strcat((char*)dns, token);
      count += strlen(token);
      token = strtok(NULL,".");
   }

   dns[count] = 0;
}

// Reverse conversion
void dns_to_host(char *answer, int *index){
   unsigned char length = (unsigned char)answer[(*index)];
   unsigned char new_index = (*index);
   int jump_amount = 1;
   bool jump = false;
   int domain_name_count = 0;

   memset(domain_name, 0, sizeof(domain_name));

   while (length != 0){
      // Checking if it's a pointer -> 11000000
      if (length == 192){

         if (jump == false)
            jump_amount++;

         new_index = answer[new_index + 1];
         length = answer[new_index];
         jump = true;   // If I jump to the pointer location
         continue;      // I don't have to increment the index
      }                 // because I am at another location in answer

      for (int i = 0; i < length; i++){
         // Not uber necesary to make domain_name global, but meh
         domain_name[domain_name_count] = answer[new_index + i + 1];
         domain_name_count++;
         if (jump == false)
            jump_amount++;
      }

      new_index += length + 1;
      length = answer[new_index];
      if (jump == false)
         jump_amount++;
      // Adding the dots in www.google.com
      domain_name[domain_name_count] = '.';
      domain_name_count++;
   }
   // jump_amount represents basically how many chars I parsed
   // from the current working location aka index
   (*index) += jump_amount;
}

// Function that composes the message - query
void make_query(char *host, char *buffer, int type){
   char *dns;
   memset(buffer, 0, BUFLEN);

   dns_header_t *dns_header     = NULL; // DNS Header
   dns_question_t *dns_question = NULL; // DNS Question

   dns_header = (dns_header_t *) buffer;
   memset(dns_header, 0, sizeof(dns_header));

   // No need to modify the other fields, they are 0 anyway
   dns_header->id       = (unsigned short)htons(getpid());
   dns_header->rd       = 1;
   dns_header->qdcount  = htons(1);

   dns = (char*) &buffer[sizeof(dns_header_t)];
   // Converting to 3www6google3com
   host_to_dns(dns, host);

   dns_question = (dns_question_t *) &buffer[ sizeof(dns_header_t)
   + strlen((const char*)dns) + 1 ];

   dns_question->qclass = htons(1);    // IN class for Internet
   dns_question->qtype = htons(type);

   // In buffer I will have now the following:
   //   - header
   //   - 3www6google3com
   //   - question

}

void parse_record(FILE *f, int *index, char *answer){
   char type[7];

   // This is name field of RR; is going into domain_name
   dns_to_host(answer, index);

   // In this struct I'm adding the other fields of RR for easy accesibility
   dns_rr_t dns_rr;
   memcpy(&dns_rr, answer + (*index), sizeof(dns_rr));

   dns_rr.type     = ntohs(dns_rr.type);
   dns_rr.class_   = ntohs(dns_rr.class_);
   dns_rr.ttl      = ntohs(dns_rr.ttl);
   dns_rr.rdlength = ntohs(dns_rr.rdlength);

   type_to_string(dns_rr.type, type);
   fprintf(f, "%s\tIN\t%s\t", domain_name, type);

   (*index) += 10; // Jumping over type, class, ttl, rdlength

   switch(dns_rr.type){
      case A:{
         printf("Found record of type A. Parsing...\n");
         unsigned char num;
         // I'm reading the adress 1 byte at a time, adding '.' when needed
         for (int i = 0; i < 4; i++){
            num = answer[(*index)];
            (*index) += 1;
            if (i != 3)
               fprintf(f, "%u.", num);
            else
               fprintf(f, "%u", num);
         }

         break;
      }

      case NS:{
         printf("Found record of type NS. Parsing...\n");
         // Reading NSDNAME like the other names (safe with pointers)
         dns_to_host(answer, index);
         fprintf(f, "%s", domain_name);

         break;
      }

      case CNAME:{
         printf("Found record of type CNAME. Parsing...\n");
         // Reading CNAME
         dns_to_host(answer, index);
         fprintf(f, "%s", domain_name);

         break;
      }

      case MX:{
         printf("Found record of type MX. Parsing...\n");
         unsigned short preference;
         // Taking preference from answer
         memcpy(&preference, answer + (*index), sizeof(preference));
         preference = ntohs(preference);
         (*index) += 2;

         fprintf(f, "%hu\t", preference);
         // Taking exchange like any other name
         dns_to_host(answer, index);
         fprintf(f, "%s", domain_name);

         break;
      }

      case SOA:{
         printf("Found record of type SOA. Parsing...\n");
         unsigned int number;
         // MNAME
         dns_to_host(answer, index);
         fprintf(f, "%s\t", domain_name);
         // RNAME
         dns_to_host(answer, index);
         fprintf(f, "%s\t", domain_name);


         // SERIAL
         memcpy(&number, answer + (*index), sizeof(number));
         fprintf(f, "%u\t", number);

         (*index) += 4;
         // REFRESH
         memcpy(&number, answer + (*index), sizeof(number));
         fprintf(f, "%u\t", number);

         (*index) += 4;
         // RETRY
         memcpy(&number, answer + (*index), sizeof(number));
         fprintf(f, "%u\t", number);

         (*index) += 4;
         // EXPIRE
         memcpy(&number, answer + (*index), sizeof(number));
         fprintf(f, "%u\t", number);

         (*index) += 4;
         // MINIMUM
         memcpy(&number, answer + (*index), sizeof(number));
         fprintf(f, "%u\t", number);

         (*index) += 4;

         break;
      }

      case TXT:{
         printf("Found record of type TXT. Parsing...\n");
         char length;
         // To explaing properly how this works, I'll make a drawing :)
         // | rdlength | length | c | c | c | length | c | c | c |
         // |    8     |    3   | c | c | c |    3   | c | c | c |

         // As long as I still have rdlength, I read a length and then
         // length chars and so on
         while(dns_rr.rdlength){
            length = answer[(*index)];
            dns_rr.rdlength --;
            (*index)++;
            for (int i = 0; i < length; i++){
               fprintf(f, "%c", answer[(*index)]);
               (*index)++;
            }
            dns_rr.rdlength -= length;
         }

         break;
      }
   }

}

bool parse_answer(char *answer, char *dns, char *ip, int type){

   int index = 6; // I'm gonna move in the answer using this index
                   // 6 stands for the beginning of answer section

   unsigned short nr_answer, nr_autority, nr_additional;
   char stype[7];
   bool ok = false;

   // I'll determine how many answer, autority and additional I received
   memcpy(&nr_answer, answer + index, sizeof(unsigned short));
   nr_answer = ntohs(nr_answer);
   index += 2;

   memcpy(&nr_autority, answer + index, sizeof(unsigned short));
   nr_autority = ntohs(nr_autority);
   index += 2;

   memcpy(&nr_additional, answer + index, sizeof(unsigned short));
   nr_additional = ntohs(nr_additional);
   index += 2;

   // Opening logfile
   FILE* f = fopen("logfile", "w");
   type_to_string(type, stype);
   fprintf(f, "; %s - %s %s\n\n", ip, dns, stype);

   // Question section; don't care about it
   dns_to_host(answer, &index);
   // Jumping over qtype, qclass as well
   index += 4;

   if (nr_answer > 0){
      ok = true;
      fprintf(f, ";; ANSWER SECTION:\n");
      while (nr_answer != 0){
         parse_record(f, &index, answer);
         fprintf(f, "\n");
         nr_answer--;
      }
      fprintf(f, "\n");
   }

   if (nr_autority > 0){
      ok = true;
      fprintf(f, ";; AUTORITY SECTION:\n");
      while (nr_autority != 0){
         parse_record(f, &index, answer);
         fprintf(f, "\n");
         nr_autority--;
      }
      fprintf(f, "\n");
   }

   if (nr_additional > 0){
      ok = true;
      fprintf(f, ";; ADDITIONAL SECTION:\n");
      while (nr_additional != 0){
         parse_record(f, &index, answer);
         fprintf(f, "\n");
         nr_additional--;
      }
   }

   return ok;
}


int main(int argc, char *argv[]){
   int sockfd, n;
   // I used aux_ip because (for unknown reasons) my ip got lost after recv
   char ip[BUFLEN], aux_ip[BUFLEN];
   // Same happens with argv[1] because of strtok so I'm making a copy 
   char dns[BUFLEN];
   strcpy(dns, argv[1]);
   // This is the body of the message, both sent and received
   char buffer[BUFLEN];
   memset(buffer, 0, BUFLEN);
   // The type of the message - A, MX, SOA etc
   int type = string_to_type(argv[2]);

   bool done = false; // If a server answers, done = true
   FILE* conf = fopen("dns_servers.conf", "r");

   // Prepare socket
   struct sockaddr_in serv_addr;

   int len = sizeof(serv_addr);
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port   = htons(53);
   // IP will be set up later

   sockfd = socket(PF_INET, SOCK_DGRAM, 0);
   if (sockfd < 0){
      fprintf(stderr, "ERROR opening socket!\n");
      return -1;
   }

   // Prepare fd_set for select()
   fd_set read_fds;
   struct timeval tv;

   FD_ZERO(&read_fds);
   FD_SET(sockfd, &read_fds);
   // Select timeout
   tv.tv_sec = WAIT;
   tv.tv_usec = 0;

   // Create message
   make_query(argv[1], buffer, type);

   while ((done == false) && (!feof(conf))){
      memset(ip, 0, BUFLEN);
      // Reading a line from .conf - the ip
      fgets(ip, BUFLEN, conf);

      // Jumping over empty lines and comments
      if (ip[0] == '#' || ip[0] == '\n'){
         continue;
      }

      // fgets does not add string terminator
      // Must do it manually
      ip[strlen(ip) - 1] = '\0';
      strcpy(aux_ip, ip);
      inet_aton(ip, &serv_addr.sin_addr);

      // Sending query
      n = sendto(sockfd, &buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr,
                 (socklen_t) sizeof(serv_addr));
      if (n < 0){
         fprintf(stderr, "ERROR sending query\n");
         return -1;
      }
      // Select with timeout
      if (select(sockfd + 1, &read_fds, NULL, NULL, &tv) <= 0){
         fprintf(stderr, "ERROR in select\n");
         return -1;
      }

      if (FD_ISSET(sockfd, &read_fds)){
         // We haz an answer!
         memset(buffer, 0, BUFLEN);

         n = recvfrom(sockfd, buffer, BUFLEN, 0,
                      (struct sockaddr *) &serv_addr.sin_addr, (socklen_t *) &len);
         if (n < 0){
            fprintf(stderr, "ERROR receiving answer\n");
            return -1;
         }
         // Parsing the answer
         done = parse_answer(buffer, dns, aux_ip, type);
      }
   }
   
   return 0;
}