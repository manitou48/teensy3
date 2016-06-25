// mbed tls port to teensy ?
//  mbedtls/config.h
#include "mbedtls/platform.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha256.h"
#include "mbedtls/aes.h"
#include "mbedtls/bignum.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"


mbedtls_md5_context md5;
mbedtls_sha256_context ctx;
mbedtls_entropy_context entropy;

void mpi_demo() {
	mbedtls_mpi E, P, Q, N, H, D, X, Y, Z;
	uint32_t t;
	int i;
  char str[1024];

	mbedtls_mpi_init( &E ); mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q ); mbedtls_mpi_init( &N );
	mbedtls_mpi_init( &H ); mbedtls_mpi_init( &D ); mbedtls_mpi_init( &X ); mbedtls_mpi_init( &Y );
	// factorial
	t=micros();
	mbedtls_mpi_read_string( &P, 10, "1" );
	for(i=2;i<=100;i++) mbedtls_mpi_mul_int( &P, &P, i );
	t=micros()-t;
	Serial.print("100! us "); Serial.println(t);
	 //mbedtls_mpi_write_file( " 100! = ",&P, 10, NULL );
   mbedtls_mpi_write_string( &P, 10, str,sizeof(str), (size_t *)&i );
   Serial.println(str);

   mbedtls_mpi_free( &E ); mbedtls_mpi_free( &P ); mbedtls_mpi_free( &Q ); mbedtls_mpi_free( &N );
   mbedtls_mpi_free( &H ); mbedtls_mpi_free( &D ); mbedtls_mpi_free( &X ); mbedtls_mpi_free( &Y );
   mbedtls_mpi_free( &Z );
}


void aes_demo() {
    // just for timing purposes  128-bit key 64 bytes cbc
    unsigned char iv[16],key[32],buff[64];
    uint32_t t;
    mbedtls_aes_context ctx;

    mbedtls_aes_init( &ctx );
    t=micros();
    mbedtls_aes_setkey_enc( &ctx, key, 128 );
    t=micros() - t;
	Serial.print("aes setkey "); Serial.println(t);

    t=micros();
    mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_ENCRYPT, 64, iv, buff, buff );
    t=micros() - t;
	Serial.print("encrypt "); Serial.println(t);
   
    mbedtls_aes_setkey_dec( &ctx, key, 128 );
    t=micros();
    mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_DECRYPT, 64, iv, buff, buff );
    t=micros() - t;
	Serial.print("decrypt "); Serial.println(t);
}


// define our own entropy source 
#if defined(__MK66FX1M0__) || defined(__MK64FX512__)
#define RNG_CR_GO_MASK                           0x1u
#define RNG_CR_HA_MASK                           0x2u
#define RNG_CR_INTM_MASK                         0x4u
#define RNG_CR_CLRI_MASK                         0x8u
#define RNG_CR_SLP_MASK                          0x10u
#define RNG_SR_OREG_LVL_MASK                     0xFF00u
#define RNG_SR_OREG_LVL_SHIFT                    8
#define RNG_SR_OREG_LVL(x)                       (((uint32_t)(((uint32_t)(x))<<RNG_SR_OREG_LVL_SHIFT))&RNG_SR_OREG_LVL_MASK)

// k64/k66 TRNG  
int my_entropy_poll(void *data, unsigned char *output, size_t len, size_t *olen  ){
	static int inited=0;
	unsigned long rng;

    if (!inited) {
        inited = 1;
        SIM_SCGC6 |= SIM_SCGC6_RNGA; // enable RNG
        RNG_CR &= ~RNG_CR_SLP_MASK;
        RNG_CR |= RNG_CR_HA_MASK;  // high assurance, not needed
    }
    RNG_CR |= RNG_CR_GO_MASK;
    while((RNG_SR & RNG_SR_OREG_LVL(0xF)) == 0); // wait
	rng = RNG_OR;
    ((void) data);
    *olen = 0;

    if( len < sizeof(unsigned long) )
        return( 0 );

    memcpy( output, &rng, sizeof(unsigned long) );
    *olen = sizeof(unsigned long);

    return( 0 );
}

#elif  defined(STM32L476xx)

//  dragonfly
int my_entropy_poll(void *data, unsigned char *output, size_t len, size_t *olen  ){
  static int inited=0;
  unsigned long rng;

    if (!inited) {
        inited = 1;
        RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;   // enable RNG
        RNG->CR = RNG_CR_RNGEN;
    }
    while((RNG->SR & RNG_SR_DRDY) == 0); // wait
    rng = RNG->DR; 
    ((void) data);
    *olen = 0;
 
    if( len < sizeof(unsigned long) )
        return( 0 );

    memcpy( output, &rng, sizeof(unsigned long) );
    *olen = sizeof(unsigned long);

    return( 0 );
}

#else

// for other could init and use systick, we'll just use micros  WEAK
// use with any platform config.h
int my_entropy_poll(void *data, unsigned char *output, size_t len, size_t *olen){
	unsigned long t=micros();   
    ((void) data);
    *olen = 0;

    if( len < sizeof(unsigned long) )
        return( 0 );

    memcpy( output, &t, sizeof(unsigned long) );
    *olen = sizeof(unsigned long);

    return( 0 );
}
#endif

// for this RNG you need ENTROPY and AES
void rng() {
    static int inited=0;
    int ret;
    static mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char buf[1024];
    uint32_t t;

	// need entropy enabled
  if(!inited){
    ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) "RANDOM_GEN", 10 );
    if( ret != 0 )
    {
		Serial.println("failed in mbedtls_ctr_drbg_seed");
        //return;
    }
    mbedtls_ctr_drbg_set_prediction_resistance( &ctr_drbg, MBEDTLS_CTR_DRBG_PR_OFF );
    inited=1;
  }
    t=micros();
    ret = mbedtls_ctr_drbg_random( &ctr_drbg, buf, sizeof( buf ) );
    t=micros()-t;
    Serial.print("rng us "); Serial.println(t);

    if( ret != 0 )
        {
            Serial.println("failed");
            //return;
        }
//    printf("%x %x %x %x   %x\n",buf[0],buf[1],buf[2],buf[3],buf[77]);
}




void setup() {
	Serial.begin(9600);
	while(!Serial);
    mbedtls_entropy_init( &entropy );
	mbedtls_entropy_add_source(&entropy, my_entropy_poll, NULL,4,1); //strong

}

void loop() {
	unsigned char buff[1024], hash[32];
	uint32_t t;
	char str[64];

	t=micros();
	mbedtls_md5_init(&md5);
	mbedtls_md5_update(&md5,buff, sizeof(buff));
	mbedtls_md5_finish(&md5,hash);
	t=micros()-t;
	sprintf(str,"md5 %lu us %lu KBs",t,1000*sizeof(buff)/t);
	Serial.println(str);

	t=micros();
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_update(&ctx,buff, sizeof(buff));
	mbedtls_sha256_finish(&ctx,hash);
	t=micros()-t;
	sprintf(str,"sha256 %lu us %lu KBs",t,1000*sizeof(buff)/t);
	Serial.println(str);


	aes_demo();
	mpi_demo();
 t=micros();
 mbedtls_entropy_func( &entropy, hash, sizeof( hash ) );
 t=micros()-t;
 Serial.print("entropy us ");Serial.println(t);
	rng();

	delay(5000);
}
