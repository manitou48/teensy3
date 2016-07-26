// K64F  CAU tests  MD5  SHA256  AES  use -lcau
// crypto assist co-processor
//  MD5 need to do padding and bookkeeping
#include "cau_api.h"

void setup() {
	Serial.begin(9600);
}

void loop() {
    unsigned int i,t,errs;
	unsigned char mdstate[16], data[10*1024];
	unsigned int shastate[8];
	unsigned char aeskey[16],keysched[4*44], in[16],out[16],iv[16];
  char str[64];

	cau_md5_initialize_output(mdstate);

	for (i=0;i<16;i++) {sprintf(str,"%02x ",mdstate[i]); Serial.print(str);} Serial.println();
	t = micros();
	cau_md5_hash_n (data,sizeof(data)/64,mdstate);  // # 64-byte blocks
	t = micros()-t;
	sprintf(str,"md5 %d bytes %u us  KBs ",sizeof(data),t); Serial.print(str);
	Serial.println(1000.*sizeof(data)/t);
	for (i=0;i<16;i++) {sprintf(str,"%02x ",mdstate[i]); Serial.print(str);} Serial.println();

	cau_sha256_initialize_output(shastate);
	for (i=0;i<8;i++) {sprintf(str,"%08x ",shastate[i]); Serial.print(str);} Serial.println();
	t = micros();
	cau_sha256_update (data,sizeof(data)/64,shastate);  // # 64-byte blocks
	t = micros()-t;
	sprintf(str,"sha256 %d bytes %u us   KBs  ",sizeof(data),t);
	Serial.print(str);
	Serial.println(1000.*sizeof(data)/t);
	for (i=0;i<8;i++) {sprintf(str,"%08x ",shastate[i]); Serial.print(str);} Serial.println();
	t = micros();
	cau_aes_set_key(aeskey,128,keysched);
	t = micros()-t;
  Serial.print("aes set key us "); Serial.println(t);
	//printf("aes set key  %u us\n",t);
	t = micros();
	cau_aes_encrypt (in,keysched,10,out);  // # 16-byte block
	t = micros()-t;
	sprintf(str,"aes %d bytes %u us  KBs  ",sizeof(in),t); Serial.print(str);
	Serial.println(1000.*sizeof(in)/t);
	cau_aes_decrypt (out,keysched,10,iv);  //  decrypt test
	errs=0;
	for (i=0;i<16;i++) if (in[i] != iv[i]) errs++;
  Serial.print("aes errs "); Serial.println(errs);
	//printf("aes errs %d\n",errs);

	// CBC XOR IV or output with plain, our sketch does 4 blocks
	t = micros();
	for (i=0;i<16;i++) in[i] = out[i] ^ iv[i];  // sort of, just for timing
	cau_aes_encrypt (in,keysched,10,out);  // # 16-byte block
	for (i=0;i<16;i++) in[i] = out[i] ^ iv[i];  // sort of, just for timing
	cau_aes_encrypt (in,keysched,10,out);  // # 16-byte block
	for (i=0;i<16;i++) in[i] = out[i] ^ iv[i];  // sort of, just for timing
	cau_aes_encrypt (in,keysched,10,out);  // # 16-byte block
	for (i=0;i<16;i++) in[i] = out[i] ^ iv[i];  // sort of, just for timing
	cau_aes_encrypt (in,keysched,10,out);  // # 16-byte block
	t = micros()-t;
	sprintf(str,"aes cbc %d bytes %u us  KBs   ",4*sizeof(in),t);
	Serial.print(str);
	Serial.println(4000.*sizeof(in)/t);
}
