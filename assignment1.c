#include <stdio.h>          /* for fprintf      */
#include <stdlib.h>         /* for exit         */
#include <unistd.h>         /* for fork         */
#include <string.h>         /* for strtok       */
#include <openssl/evp.h>    /* for crypto stuff */
#include <openssl/aes.h>

int parse_args(int argc,  char *argv[ ], int *np)
{
  if ( (argc != 3) || ((*np = atoi (argv[1])) <= 0) ) {
    fprintf (stderr, "Usage: %s nprocs partial_key\n", argv[0]);
    return(-1); };
  return(0); 
}
int make_trivial_ring()
{   
  int   fd[2];
  if (pipe(fd) == -1) 
    return(-1); 
  if ((dup2(fd[0], STDIN_FILENO) == -1) ||
      (dup2(fd[1], STDOUT_FILENO) == -1)) 
    return(-2); 
  if ((close(fd[0]) == -1) || (close(fd[1]) == -1))   
    return(-3); 
  return(0); 
}
int add_new_node(int *pid)
{
  int fd[2];
  if (pipe(fd) == -1) 
    return(-1); 
  if ((*pid = fork()) == -1)
    return(-2); 
  if(*pid > 0 && dup2(fd[1], STDOUT_FILENO) < 0)
    return(-3); 
  if (*pid == 0 && dup2(fd[0], STDIN_FILENO) < 0)
    return(-4); 
  if ((close(fd[0]) == -1) || (close(fd[1]) == -1)) 
    return(-5);
  return(0);
}
int aes_init(unsigned char *key_data, int key_data_len, EVP_CIPHER_CTX *e_ctx, EVP_CIPHER_CTX *d_ctx)
{
  int i; //nrounds = 5;
  unsigned char key[32], iv[32];
  //Some robust programming to start with
  //Only use most significant 32 bytes of data if > 32 bytes
  if(key_data_len > 32) key_data_len =32;

  //Copy bytes to the front of the key array
  for (i=0;i<key_data_len; i++){
     key[i] = key_data[i];
     iv[i] = key_data[i];
  }
  for (i=key_data_len;i<32;i++){
     key[i] = 0;
     iv[i] = 0;
  } 
  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_init(d_ctx);
  EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

  return 0;
}
unsigned char *aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len)
{
  /* plaintext will always be equal to or lesser than length of ciphertext*/
  int p_len = *len, f_len = 0;
  unsigned char *plaintext = malloc(p_len);
  
  EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
  EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
  EVP_DecryptFinal_ex(e, plaintext+p_len, &f_len);

  //*len = p_len + f_len;
  return plaintext;
}
int main(int argc,  char *argv[])
{
  int    i;               /* number of this process (starting with 1)   */
  int    childpid;        /* indicates process should spawn another     */ 
  int    nprocs;          /* total number of processes in ring          */ 

  unsigned char trialkey[32]; /* whatever key we're testing */
  char *plaintext;            /*  */

  // check command line arguments
  if(parse_args(argc,argv,&nprocs) < 0) exit(EXIT_FAILURE);

  //read plain text from plain.txt
  FILE *myplainfile;    
  myplainfile=fopen("plain.txt","r");
  char plain_in[4096]; 
  fread(plain_in, 28, 1, myplainfile);

  //print plain text to console
  fprintf(stderr,"\nPlain: %s\n",plain_in);

  //read the ciphertext from cipher.txt
  int cipher_length = 32;
  FILE *mycipherfile;    
  mycipherfile=fopen("cipher.txt","r");
  unsigned char cipher_in[4096];
  fread(cipher_in, cipher_length, 1, mycipherfile);

  //print cipher text to console
  fprintf(stderr,"\nCipher: %s\n",cipher_in);

  //grab the partial key from command line
  unsigned char *key_data;
  int  key_data_len;
  key_data = (unsigned char *)argv[2];
  key_data_len = strlen(argv[2]);

  //print partial key to console
  fprintf(stderr,"\nPartial key:  %s\n",key_data);

  //256 bit key
  int key_length = 32;
  if(key_data_len > key_length) key_data_len = key_length;

  //Copy known bytes to front of key array
  for (i=0;i<key_data_len; i++){
    trialkey[i] = key_data[i];
  }

  //fill remainder of key with 0s
  for (i = key_data_len; i < key_length; i++){
    trialkey[i] = 0;
  }

  //no. missing chars
  int missing_data;
  missing_data = key_length - key_data_len;

  //number of possible keys
  unsigned long maxSpace = ((unsigned long)1 << ((missing_data)*8))-1;

  //construct the ring of processes
  if (make_trivial_ring() < 0){
    perror("Could not make trivial ring");
    exit(EXIT_FAILURE); };
  for (i = 1; i < nprocs;  i++) {
    if(add_new_node(&childpid) < 0){
      perror("Could not add new node to ring");
      exit(EXIT_FAILURE); 
    }
    if (childpid) break; 
  }
  //all processes do this:
  unsigned long counter;  //iterate through every possible trial key
  unsigned char key[32];  //final key will be stored here

  for (counter = i-1; counter < maxSpace; counter=counter+nprocs){
    int y = 0;
    //contruct the trial key
    for (y = 0; y < missing_data; y++){
      trialkey[31-y] = (unsigned char) (counter >> (y*8));
    }
    //initialise cipher etc
    EVP_CIPHER_CTX en, de;
    if (aes_init(trialkey, key_length, &en, &de)) {
      printf("Couldn't initialize AES cipher\n");
      return -1;
    }
    plaintext = (char *)aes_decrypt(&de, (unsigned char *)cipher_in, &cipher_length);
    EVP_CIPHER_CTX_cleanup(&en);
    EVP_CIPHER_CTX_cleanup(&de);

    //check trial key
    if (strncmp(plaintext, plain_in, 28)){
    }else{
      for(y=0;y<32;y++){
        key[y] = trialkey[y]; //set trial key to key
      }
      //write key to pipe
      if (write(STDOUT_FILENO, key, 32) < 0){
        perror("Could not write key to pipe");
        exit(EXIT_FAILURE);
      }
      break;
    }
  }
  int y = 0;
  if (read(STDIN_FILENO, key, 32) > 0){
    if (i == 1){
      fprintf(stderr,"\nComplete key: ");
      for(y=0;y<32;y++){
        fprintf(stderr,"%c",key[y]); 
      }
      fprintf(stderr,"\n");
    } else
      if (write(STDOUT_FILENO, key, 32) < 0) perror("Could not write key to pipe");
  }
  exit(0);
}