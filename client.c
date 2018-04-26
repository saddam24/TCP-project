#include <stdio.h>
#include<stdio.h>
#include<string.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<math.h>
#include<unistd.h>

//This Function Encodes and store in a buffer the information being passed
int BUDP_Encoder(char * dst, uint64_t val, int offset, int size) {
    int i;
    for (i = 0; i < size; i++) {
    dst[offset++] = (val >> ((size - 1) - i) *8);}

    return offset;
}

//This Function Decodes the information stored in a buffer based on the given position
uint32_t BUDP_Decoder(char * met,int offset,int size){

   uint32_t result=0;
   int i;
   for (i=0;i<(size-1);i++){
     result+=met[i+offset]<<(size-1-i)*8;
   }
   return result=result+(met[size+offset-1]&0XFF);
}

//This Function is used to determine the number of segments the data will be divided into
int NumberChunks(FILE * fp, int BlockSize){
  int position;
  int NumberBlocks;
  fseek(fp, 0L, SEEK_END);
  position = ftell(fp);
  NumberBlocks= ceil((float)position/(float)BlockSize);
  rewind(fp);
  return NumberBlocks;
}

//This Function is used to calculate the checksum of the data
uint16_t BUDP_chksum(const char *pbff, int nbytes)
{
    uint32_t sum=0;
    uint8_t pad=0;
    while(nbytes>1) {
        sum=sum+(((*pbff)<<8)+(*(pbff+1)));
        nbytes-=2;
        pbff=pbff+2;
    }

    if(nbytes==1) {
     sum+=((*pbff)<<8)+(pad);             //If Data is odd (We add 1 byte Padding)
    }
    while (sum>>16){
		sum = (sum & 0xFFFF)+(sum >> 16);
    }
    uint16_t checksum=(uint16_t)sum;
    return ~checksum;                    // We return the 16 bit sum's complement
}


int main(){
//Variable Declaration
char       header[16];
char       data[496];
char       chunkBuf[512];
char       buf[1024];
char       tempbuf[1550];
int        sd;
struct     sockaddr_in servaddr;
int        serv_len;
uint32_t   sequenceno;
uint32_t   c_ackn;
uint16_t   checksum;
uint32_t   windowsize_;
uint16_t   c_options;
int        expack;
int        recvackn;
int        recvwindow;
int        remainder_packets;
int        segment_transmited;
int        packet;
int        BytesRead;
int        bytesSent;
int        bytesReceived;
FILE *fp;
//Initialization of variables
memset(chunkBuf,0,sizeof(chunkBuf));
memset(tempbuf,0,sizeof(tempbuf));
memset(data,0,sizeof(data));
memset(header,0,sizeof(header));
c_ackn=0;
sequenceno=0;
windowsize_=496;
expack=0;  
c_options=0; 
recvackn=0;
segment_transmited=0;
packet=1;
   
        /* Create UDP socket */
        sd=socket(AF_INET,SOCK_DGRAM,0);

		/*Set timeout value */
		struct timeval timeout;
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		
	if (setsockopt (sd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(timeout)) < 0){ //option_length
        	    printf("setsockopt failed\n");}
		
// SO_RCVTIMEO with the Structure timeval is used to define a timeout for Non-Blocking Socket

 if(sd==-1)
        {
                printf("Creating socket is failed.\n");
                exit(0);
        }
 else{
        //Set IP address and port number 
        bzero(&servaddr,sizeof(servaddr)); //this line initializes serv_addr to zeros
        //The bzero() function shall place n zero-valued bytes in the area pointed to by 1st argument.
        servaddr.sin_family=AF_INET; //contains a code for the address family
        servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
        servaddr.sin_port=htons(atoi("5000"));
        //function htons() converts a port number in host byte order to a port number in network byte order.
	serv_len = sizeof(servaddr);
  
   printf("\n*********************************");
   printf("\nBUDP CLIENT by Jorge Medina\n");
   printf("*********************************\n\n");
   

        fp=fopen("input.jpeg","r");

        if(fp==NULL){
          printf("ERROR:Opening the Data File\n");
          exit(1);
        }
        int chunkNo=NumberChunks(fp,496);  //We calculate the number of segments the data will be divided into
        printf("THE DATA WILL BE SEGMENTED INTO %d SEGMENTS.\n\n",chunkNo);
 

 while(1){
 int i;
// Start Making Packets to be sent over the wire
for(i=0;i<packet;i++){
    int u=0;
    BytesRead=fread(data,1,496,fp);
    checksum=BUDP_chksum(data,BytesRead);
    printf("Packet Information: Sequenceno=%d\n",sequenceno);
    printf("Packet Information: checksum=%d\n",checksum);
    printf("Packet Information: windowsize_=%d\n",windowsize_);
//We encode the information into byte characters to be sent
   
    u=BUDP_Encoder(header,sequenceno,u,sizeof(uint32_t));
    u=BUDP_Encoder(header,c_ackn,u,sizeof(uint32_t));
    u=BUDP_Encoder(header,checksum,u,sizeof(uint16_t));
    u=BUDP_Encoder(header,c_options,u,sizeof(uint16_t));
    u=BUDP_Encoder(header,windowsize_,u,sizeof(uint32_t));
    memcpy(chunkBuf,header,16);
    memcpy(chunkBuf+16,data,BytesRead);
//We encode the information and save it temporality in a buffer, to be ready in case of retransmision   
    u=0;
    u=BUDP_Encoder(&tempbuf[i*(502)],sequenceno,u,sizeof(uint32_t));
    u=BUDP_Encoder(&tempbuf[i*(502)],BytesRead,u,sizeof(uint16_t));
    memcpy(&tempbuf[6+i*502],data,BytesRead);
//Sending Data                
    bytesSent = sendto(sd,chunkBuf, BytesRead+16,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
    printf("Packet Information: %d bytes were sent.\n\n",bytesSent);
    sequenceno++;  //We increase the sequence Number
}
    

//Receiving Data from Server

    bytesReceived = recvfrom(sd, &buf, 1024,0,(struct sockaddr *)&servaddr,&serv_len);
//In case of Packet loss,(Time Out) we procede to send back the packet corresponding to the ackn received.
    while(bytesReceived==-1){
          int t;
          int p=recvackn;
          for(t=0;t<sizeof(tempbuf);t+=502){   // We loop until we find the sequencenumber needed
             printf("\nPACKET RETRANSMISION: Due to Packet Loss\n");
             if(p==BUDP_Decoder(tempbuf+t,0,4)){
              int datatemp_size=BUDP_Decoder(tempbuf+t,4,2);
              int u=0;
              char datatemp[datatemp_size];
              memset(datatemp,0,datatemp_size); 
              memcpy(datatemp,tempbuf+t+6,datatemp_size);
              checksum=BUDP_chksum(datatemp,datatemp_size);
              u=BUDP_Encoder(header,recvackn,u,sizeof(uint32_t));
              u=BUDP_Encoder(header,c_ackn,u,sizeof(uint32_t));
              u=BUDP_Encoder(header,checksum,u,sizeof(uint16_t));
              u=BUDP_Encoder(header,c_options,u,sizeof(uint16_t));
              u=BUDP_Encoder(header,windowsize_,u,sizeof(uint32_t));
              memcpy(chunkBuf,header,16);
              memcpy(chunkBuf+16,datatemp,BUDP_Decoder(tempbuf+t,4,2));
              bytesSent = sendto(sd,chunkBuf,BUDP_Decoder(tempbuf+t,4,2)+16,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
              printf("Packet Information Sequenceno %d\n",recvackn);
              printf("Packet Information:checksum: %d\n",checksum);              
              printf("Packet Information Windowsize %d\n",windowsize_);
              printf("Packet Information: %d bytes were sent.\n\n",bytesSent);
              p++;
              bytesReceived = recvfrom(sd, &buf, 1024,0,(struct sockaddr *)&servaddr,&serv_len);
              recvackn=BUDP_Decoder(buf,4,4);
              break;
             }
           
            }

              break;  //we break the while loop if there is not match of the received ack and the sequencenumbers store.
    }
     recvackn=BUDP_Decoder(buf,4,4);
     recvwindow=BUDP_Decoder(buf,12,4);
     expack=expack+packet;
     printf("INFO: The Expected ackn is: %d and the Received ackn is: %d \n\n",expack,recvackn);

 //In case the expected ack matches the received ack we increment the number of packets to be sent,taking into consideration the windowsize received
    if(expack==recvackn){
     segment_transmited+=packet;
     remainder_packets=chunkNo-segment_transmited;
     windowsize_+=(segment_transmited)*496;  //Slow start sending packets exponentially without surpassing the Received Window size.
    if((windowsize_>recvwindow)&&(recvwindow<=sizeof(tempbuf))){             // We adjust to the size of the window received
         windowsize_=recvwindow;    
      }
     else if(recvwindow>sizeof(tempbuf)){
         windowsize_=sizeof(tempbuf);
      }
      packet=(windowsize_/496);              //Calculate the number of packets to be sent
      windowsize_=496*packet;
    
    }
//In case the expected ack does not match the corresponding received ack, we send the packet with sequence corresponding to the last ack Received.     
      else{
          printf("\nPACKET RETRANSMISION:ACK received (#%d) differs from the expected (#%d) ACKN\n",recvackn,expack);
          printf("...Fast Packet Retransmision Information\n\n");
          int t;
          int p=recvackn;
          for(t=0;t<sizeof(tempbuf);t+=502){  // We loop until we find the sequencenumber needed in the temp Data Buffer
             if(p==BUDP_Decoder(tempbuf+t,0,4)){
              int datatemp_size=BUDP_Decoder(tempbuf+t,4,2);
              int u=0;
              int counter=0;
              char datatemp[datatemp_size];
              memset(datatemp,0,datatemp_size);    
              memcpy(datatemp,tempbuf+t+6,datatemp_size);
              windowsize_=496;                           //We reduce the windowsize to the minimum possible/one packet size
              checksum=BUDP_chksum(datatemp,datatemp_size);
              u=BUDP_Encoder(header,recvackn,u,sizeof(uint32_t));
              u=BUDP_Encoder(header,c_ackn,u,sizeof(uint32_t));
              u=BUDP_Encoder(header,checksum,u,sizeof(uint16_t));
              u=BUDP_Encoder(header,c_options,u,sizeof(uint16_t));
              u=BUDP_Encoder(header,windowsize_,u,sizeof(uint32_t));
              memcpy(chunkBuf,header,16);
              memcpy(chunkBuf+16,datatemp,BUDP_Decoder(tempbuf+t,4,2));
              bytesSent = sendto(sd,chunkBuf, BUDP_Decoder(tempbuf+t,4,2)+16,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
              printf("Packet Information: Sequence %d\n",recvackn);
              printf("Packet Information: checksum %d\n",checksum);
              printf("Packet Information: Windowsize %d\n",windowsize_);
              printf("Packet Information: %d bytes were sent. \n",bytesSent);  
              bytesReceived = recvfrom(sd, &buf, 1024,0,(struct sockaddr *)&servaddr,&serv_len);
//We loop the sending and Receiving process and close the transfer, in case a maximum of time outs for the recvfrom call function, get reached.              
             do{
                
                bytesSent = sendto(sd,chunkBuf, BUDP_Decoder(tempbuf+t,4,2)+16,0,(struct sockaddr *)&servaddr,sizeof(servaddr));
                bytesReceived = recvfrom(sd, &buf, 1024,0,(struct sockaddr *)&servaddr,&serv_len);
                counter++;
                printf("....time out\n");
              if(counter==10){
              
                 printf("\nERROR: The transfer stopped due to maximum amount of time outs allowed!!\n\n");
                 return 0;}
                }while(bytesReceived==-1);
              p++;
              recvackn=BUDP_Decoder(buf,4,4);
              printf("INFO: The ackn received after retransmision is:%d\n\n",recvackn);
              if(recvackn==expack){  //We exit the loop until we have retransmited the packets necessary.
                 segment_transmited+=packet;
                 remainder_packets=chunkNo-segment_transmited;
                 break;}

              }//if ackn==mydecoder

                                  }//for(t=0;t<tempbuf...*/

     }//Else print("Idunno"
       memset(tempbuf,0,sizeof(tempbuf));
       if(remainder_packets<packet){
       packet=remainder_packets;
       windowsize_=496*packet;

     }

  
    if(segment_transmited>=chunkNo){   //We exit the program when the number of segmented transmited reach the numbers of segments that data was divided into
    printf("\nThe Client has Successfully Transfered the data intended\n");
    printf("**************************************************\n\n");
    break;                             //We exit the main while loop because the data was transfered.
  }  


}
	

	}
        close(sd);                    //close socket descriptor
        return 0;
}

