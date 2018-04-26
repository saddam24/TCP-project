#include <stdio.h>
#include<stdio.h>
#include<string.h>
#include<sys/time.h>
#include<netinet/in.h>
#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>


//This Function Decodes the information stored in a buffer based on the given position
uint32_t BUDP_Decoder(char * met,int offset,int size){

   uint32_t result=0;
   int i;
   for (i=0;i<(size-1);i++){
     result+=met[i+offset]<<(size-1-i)*8;
   }
   return result=result+(met[size+offset-1]&0XFF);

}

//This Function Encodes the information and is used to place it in a buffer
int BUDP_Encoder(char * dst, uint64_t val, int offset, int size) {
    int i;
    for (i = 0; i < size; i++) {
    dst[offset++] = (val >> ((size - 1) - i) *8);}

    return offset;
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

int main()
{
//Variable Declaration	
       int sock, client_len;
	struct sockaddr_in clientaddr; 
	char buf[512];       //Buffer used to receive the data sent by the client.
        char buf1[16];       //Buffer to be used to store the information to be sent to the client
        uint32_t sequenceno;
        uint32_t ackn;
        uint16_t cksum;
        uint16_t s_options;
        uint32_t windowsize_;
	uint32_t recvseq;
	uint32_t recvackn;
        uint16_t recvcksum;
	uint32_t recvwindow;
        uint16_t calcksum;
	int Maxwindowsize;
        int bytesReceived;
	int bytewritten;
	int bytesSent;
        FILE *fp;
//Initialization of variables        
        memset(buf,0,sizeof(buf));
        memset(buf1,0,sizeof(buf1));
        ackn=0;
        Maxwindowsize=1550;   //Maximum data the server can received simultaneously

//Create socket 
	sock=socket(AF_INET,SOCK_DGRAM,0);
	if(sock==-1)
	{
		printf("Creating socket is failed.\n");
		exit(0);
	}

	//Set address and port number
	bzero(&clientaddr,sizeof(clientaddr));
	clientaddr.sin_family=AF_INET;
	clientaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	clientaddr.sin_port=htons(atoi("5000"));
	bind(sock,(struct sockaddr *)&clientaddr,sizeof(clientaddr));
	
	client_len = sizeof(clientaddr);
	bzero(&buf,sizeof(buf));
        printf("\n***************\n");
        printf("BUDP SERVER by Jorge Medina\n");
        printf("***************\n\n");

//We set a infinite loop to get the data from the recvfrom call 
	for (;;)
	{
            int n=0;	
		/* Receive datagram */
             while(1){             // looping until we receive the total of segment dictated by the recvwindows
		bytesReceived = recvfrom(sock, &buf, 2000,0,(struct sockaddr *)&clientaddr,&client_len);
                n++;
                //printf("The value of n is %d\n",n);
                recvwindow=BUDP_Decoder(buf,12,4);
                if(recvwindow==496){n=1;}
                recvseq=BUDP_Decoder(buf,0,4);
		if(bytesReceived > 0){		
//We proceed to analyze the packets received whose sequence number matches the last ack sent
                      if(ackn==recvseq){  
                        
                        char recvdata[bytesReceived-16];
                        memset(recvdata,0,bytesReceived);
                        recvcksum=BUDP_Decoder(buf,8,2); 
                        memcpy(recvdata,buf+16,bytesReceived-16);
                        calcksum=BUDP_chksum(recvdata, bytesReceived-16); // We calculate the checksum of the received chunk
                        recvseq=BUDP_Decoder(buf,0,4);
                        recvackn=BUDP_Decoder(buf,4,4);
                        recvwindow=BUDP_Decoder(buf,12,4);
                        printf("Packet Information: recvseq %d\n",recvseq);
                        printf("Packet Information:recvwindow: %d\n",recvwindow);
                        printf("Packet Information:Checksum Received: %d ; ChecksumCalculated: %d\n",recvcksum,calcksum);
                    if(calcksum==recvcksum){   //We check for error in data
                            
                             fp=fopen("tengo.jpeg","a");  //We open the file to get ready for the copy process
                             bytewritten=fwrite(recvdata,1,bytesReceived-16,fp); //We proceed to copy the data received to the output file.
                             fclose(fp);
                             ackn+=1;
                             printf("INFO:Checksum Match\n");
                             printf("Packet Information: %d Bytes has been written to the file.\n\n",bytewritten);

                       }//if calcksu==recvcksum
                     else{
                             printf("Info:Checksum is incorrect\n");

                         }//else checksum

                    }//fin del ackn==recvseq

//We proceed to analyze the packets received whose sequence number does not match the last ack sent
                    else if(recvseq!=ackn){  
                           printf("INFO: A Packet with Sequenceno(%d) was received, and the last ACKN sent is:(%d)\n",recvseq,ackn);
                           printf("Packet Information:RecvSequenceno\n",recvseq);
                           printf("Packet Information:recvwindow :%d\n\n",recvwindow);
                      }//else ackn==recvseq */

                       //printf("The value for N is: %d and the value of the compare is %d\n",n,recvwindow/496);


                }// bytesReceived>0
                
              if(n==recvwindow/496){// if we get all of the packets according to the recwindow we close and proceed to send the ack
                 if(ackn>recvseq){windowsize_=Maxwindowsize;} 
                 else{windowsize_=496; //We reduce the windowsize after receiving all packets which sequence number doesnt match the last ack sent, and resend the last ackn

                    }//else else{windowsize
                    break;
              }//if n==recvwindow/496
            }//while loop

//We proceed to send the information regarding the packets received (Ack)
                printf("INFO: The ACKN #%d was sent.\n",ackn);
                printf("INFO: The window has now set to #%d Bytes.\n\n",windowsize_);
                int u=0;
                u=BUDP_Encoder(buf1,sequenceno,u,sizeof(uint32_t));
                u=BUDP_Encoder(buf1,ackn,u,sizeof(uint32_t));
                u=BUDP_Encoder(buf1,cksum,u,sizeof(uint16_t));
                u=BUDP_Encoder(buf1,s_options,u,sizeof(uint16_t));
                u=BUDP_Encoder(buf1,windowsize_,u,sizeof(uint32_t));
                bytesSent = sendto(sock,buf1, 16,0,(struct sockaddr *)&clientaddr,client_len);



	}//for loop

	return 0;
}
