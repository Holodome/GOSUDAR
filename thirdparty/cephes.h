#if !defined(CEPHES_H)

#define mtherr(_a, _b) ((void)_a)

// constf.c
#ifdef DEC
/* MAXNUMF = 2^127 * (1 - 2^-24) */
float MAXNUMF = 1.7014117331926442990585209174225846272e38;
float MAXLOGF = 88.02969187150841;
float MINLOGF = -88.7228391116729996; /* log(2^-128) */
#else
/* MAXNUMF = 2^128 * (1 - 2^-24) */
float MAXNUMF = 3.4028234663852885981170418348451692544e38;
float MAXLOGF = 88.72283905206835;
float MINLOGF = -103.278929903431851103; /* log(2^-149) */
#endif

float LOG2EF = 1.44269504088896341;
float LOGE2F = 0.693147180559945309;
float SQRTHF = 0.707106781186547524;
float PIF = 3.141592653589793238;
float PIO2F = 1.5707963267948966192;
float PIO4F = 0.7853981633974483096;
float MACHEPF = 5.9604644775390625E-8;


//
// sinf.c
//

static float FOPI = 1.27323954473516;

extern float PIO4F;
/* Note, these constants are for a 32-bit significand: */
/*
static float DP1 =  0.7853851318359375;
static float DP2 =  1.30315311253070831298828125e-5;
static float DP3 =  3.03855025325309630e-11;
static float lossth = 65536.;
*/

/* These are for a 24-bit significand: */
static float DP1 = 0.78515625;
static float DP2 = 2.4187564849853515625e-4;
static float DP3 = 3.77489497744594108e-8;
static float lossth = 8192.;
static float T24M1 = 16777215.;

static float sincof[] = {
    -1.9515295891E-4,
    8.3321608736E-3,
    -1.6666654611E-1
};
static float coscof[] = {
    2.443315711809948E-005,
    -1.388731625493765E-003,
    4.166664568298827E-002
};

float sinf( float xx )
{
    float *p;
    float x, y, z;
    unsigned long j;
    int sign;
    
    sign = 1;
    x = xx;
    if( xx < 0 )
	{
        sign = -1;
        x = -xx;
	}
    if( x > T24M1 )
	{
        // mtherr( "sinf", TLOSS );
        return(0.0);
	}
    j = FOPI * x; /* integer part of x/(PI/4) */
    y = j;
    /* map zeros to origin */
    if( j & 1 )
	{
        j += 1;
        y += 1.0;
	}
    j &= 7; /* octant modulo 360 degrees */
    /* reflect in x axis */
    if( j > 3)
	{
        sign = -sign;
        j -= 4;
	}
    
    if( x > lossth )
	{
        // mtherr( "sinf", PLOSS );
        x = x - y * PIO4F;
	}
    else
	{
        /* Extended precision modular arithmetic */
        x = ((x - y * DP1) - y * DP2) - y * DP3;
	}
    /*einits();*/
    z = x * x;
    if( (j==1) || (j==2) )
	{
        /* measured relative error in +/- pi/4 is 7.8e-8 */
        /*
            y = ((  2.443315711809948E-005 * z
              - 1.388731625493765E-003) * z
              + 4.166664568298827E-002) * z * z;
        */
        p = coscof;
        y = *p++;
        y = y * z + *p++;
        y = y * z + *p++;
        y *= z * z;
        y -= 0.5 * z;
        y += 1.0;
	}
    else
	{
        /* Theoretical relative error = 3.8e-9 in [-pi/4, +pi/4] */
        /*
            y = ((-1.9515295891E-4 * z
                 + 8.3321608736E-3) * z
                 - 1.6666654611E-1) * z * x;
            y += x;
        */
        p = sincof;
        y = *p++;
        y = y * z + *p++;
        y = y * z + *p++;
        y *= z * x;
        y += x;
	}
    /*einitd();*/
    if(sign < 0)
        y = -y;
    return( y);
}


/* Single precision circular cosine
 * test interval: [-pi/4, +pi/4]
 * trials: 10000
 * peak relative error: 8.3e-8
 * rms relative error: 2.2e-8
 */

float cosf( float xx )
{
    float x, y, z;
    int j, sign;
    
    /* make argument positive */
    sign = 1;
    x = xx;
    if( x < 0 )
        x = -x;
    
    if( x > T24M1 )
	{
        // mtherr( "cosf", TLOSS );
        return(0.0);
	}
    
    j = FOPI * x; /* integer part of x/PIO4 */
    y = j;
    /* integer and fractional part modulo one octant */
    if( j & 1 )	/* map zeros to origin */
	{
        j += 1;
        y += 1.0;
	}
    j &= 7;
    if( j > 3)
	{
        j -=4;
        sign = -sign;
	}
    
    if( j > 1 )
        sign = -sign;
    
    if( x > lossth )
	{
        // mtherr( "cosf", PLOSS );
        x = x - y * PIO4F;
	}
    else
    /* Extended precision modular arithmetic */
        x = ((x - y * DP1) - y * DP2) - y * DP3;
    
    z = x * x;
    
    if( (j==1) || (j==2) )
	{
        y = (((-1.9515295891E-4 * z
               + 8.3321608736E-3) * z
              - 1.6666654611E-1) * z * x)
            + x;
	}
    else
	{
        y = ((  2.443315711809948E-005 * z
              - 1.388731625493765E-003) * z
             + 4.166664568298827E-002) * z * z;
        y -= 0.5 * z;
        y += 1.0;
	}
    if(sign < 0)
        y = -y;
    return( y );
}

//
// tanf.c
//


extern float MAXNUMF;

static float tancotf( float xx, int cotflg )
{
    float x, y, z, zz;
    long j;
    int sign;
    
    
    /* make argument positive but save the sign */
    if( xx < 0.0 )
	{
        x = -xx;
        sign = -1;
	}
    else
	{
        x = xx;
        sign = 1;
	}
    
    if( x > lossth )
	{
        //if( cotflg )
        //mtherr( "cotf", TLOSS );
        //else
        //mtherr( "tanf", TLOSS );
        return(0.0);
	}
    
    /* compute x mod PIO4 */
    j = FOPI * x; /* integer part of x/(PI/4) */
    y = j;
    
    /* map zeros and singularities to origin */
    if( j & 1 )
	{
        j += 1;
        y += 1.0;
	}
    
    z = ((x - y * DP1) - y * DP2) - y * DP3;
    
    zz = z * z;
    
    if( x > 1.0e-4 )
	{
        /* 1.7e-8 relative error in [-pi/4, +pi/4] */
        y =
        ((((( 9.38540185543E-3 * zz
             + 3.11992232697E-3) * zz
            + 2.44301354525E-2) * zz
           + 5.34112807005E-2) * zz
          + 1.33387994085E-1) * zz
         + 3.33331568548E-1) * zz * z
            + z;
	}
    else
	{
        y = z;
	}
    
    if( j & 2 )
	{
        if( cotflg )
            y = -y;
        else
            y = -1.0/y;
	}
    else
	{
        if( cotflg )
            y = 1.0/y;
	}
    
    if( sign < 0 )
        y = -y;
    
    return( y );
}


float tanf( float x )
{
    
    return( tancotf(x,0) );
}

float cotf( float x )
{
    
    if( x == 0.0 )
	{
        // mtherr( "cotf", SING );
        return( MAXNUMF );
	}
    return( tancotf(x,1) );
}

// asinf

extern float PIF, PIO2F;

float Sqrt(f32 v);

float sqrtf( float );
float asinf( float xx )
{
    float a, x, z;
    int sign, flag;
    
    x = xx;
    
    if( x > 0 )
	{
        sign = 1;
        a = x;
	}
    else
	{
        sign = -1;
        a = -x;
	}
    
    if( a > 1.0 )
	{
        mtherr( "asinf", DOMAIN );
        return( 0.0 );
	}
    
    if( a < 1.0e-4 )
	{
        z = a;
        goto done;
	}
    
    if( a > 0.5 )
	{
        z = 0.5 * (1.0 - a);
        x = Sqrt( z );
        flag = 1;
	}
    else
	{
        x = a;
        z = x * x;
        flag = 0;
	}
    
    z =
    (((( 4.2163199048E-2 * z
        + 2.4181311049E-2) * z
       + 4.5470025998E-2) * z
      + 7.4953002686E-2) * z
     + 1.6666752422E-1) * z * x
        + x;
    
    if( flag != 0 )
	{
        z = z + z;
        z = PIO2F - z;
	}
    done:
    if( sign < 0 )
        z = -z;
    return( z );
}

float acosf( float x )
{
    
    if( x < -1.0 )
        goto domerr;
    
    if( x < -0.5) 
        return( PIF - 2.0 * asinf( Sqrt(0.5*(1.0+x)) ) );
    
    if( x > 1.0 )
	{
        domerr:	mtherr( "acosf", DOMAIN );
        return( 0.0 );
	}
    
    if( x > 0.5 )
        return( 2.0 * asinf(  Sqrt(0.5*(1.0-x) ) ) );
    
    return( PIO2F - asinf(x) );
}

#define CEPHES_H 1
#endif 
