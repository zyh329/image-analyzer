#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "common.h"
#include "iaio.h"
#include "image_analyzer.h"
#include "ia_sequence.h"
#include "analyze.h"

/* functions that are not yet fully operable *****************************************
void zoom_in(ia_pixel_t* im, ia_pixel_t* out)
{
    int i, j, height, width;
    height = HEIGHT*2;
    width = WIDTH*2;
    for(i = 0; i < HEIGHT; i++)
    {
        for(j = 0; j < WIDTH; j++)
        {
            out[width*(i*2)+(j*2)] = im[WIDTH*(i)+(j)];
        }
    }
    
    for(i = 0; i < height; i++)
    {
        for(j = 0; j < width; j++)
        {
            if(j%2 != 0 && i%2 != 0)
                out[width*i+j] = (((out[width*(i-1)+(j-1)] + out[width*(i+1)+(j+1)])/2) + ((out[width*(i+1)+(j-1)] + out[width*(i-1)+(j+1)])/2))/2;
            else if(j%2 == 0 && i%2 != 0)
                out[width*i+j] = (out[width*(i-1)+j]+out[width*(i+1)+j])/2;
            else if(i%2 == 0 && j%2 != 0)
                out[width*i+j] = (out[width*i+(j-1)]+out[width*i+(j+1)])/2;
        }
    }
}

void k(ia_pixel_t* im, ia_pixel_t* out)
{
    int i, j;
    ia_pixel_t c = 0;
    ia_pixel_t max = -99999999, min = 999999;
    for(i = 1; i < HEIGHT-1; i++)
    {
        for(j = 1; j < WIDTH-1; j++)
        {
            c = 0;
// * DIFFERENCE IN SURROUNDINGS *
            c += fabs(im[WIDTH*(i-1)+(j-1)] - im[WIDTH*(i+0)+(j-1)]);
                        c += fabs(im[WIDTH*(i+0)+(j-1)] - im[WIDTH*(i+1)+(j-1)]);
                        c += fabs(im[WIDTH*(i+1)+(j-1)] - im[WIDTH*(i+1)+(j+0)]);
                        c += fabs(im[WIDTH*(i+1)+(j+0)] - im[WIDTH*(i+1)+(j+1)]);
                        c += fabs(im[WIDTH*(i+1)+(j+1)] - im[WIDTH*(i+0)+(j+1)]);
                        c += fabs(im[WIDTH*(i+0)+(j+1)] - im[WIDTH*(i-1)+(j+1)]);
                        c += fabs(im[WIDTH*(i-1)+(j+1)] - im[WIDTH*(i-1)+(j+0)]);
                        c += fabs(im[WIDTH*(i-1)+(j+0)] - im[WIDTH*(i-1)+(j-1)]);
            out[WIDTH*i+j] = (8-c) * 1.333;
            if(out[WIDTH*i+j] > max) max = out[WIDTH*i+j];
            if(out[WIDTH*i+j] < min) min = out[WIDTH*i+j];
        }
    }
//fprintf(stderr, "%f\n", max);
//fprintf(stderr, "%f\n", min);
    max -= min;
    max = 255/max;
    for(i = 1; i < HEIGHT; i++)
    {
        for(j = 1; j < WIDTH; j++)
        {
            out[WIDTH*i+j] -= min;
            out[WIDTH*i+j] *= max;
        }
    }
}

int* q(ia_pixel_t* im)
{
        int window, s, width, height, x, y, h, k, *q, *q1, *q2; //, stddev, n, th, tk;
        ia_pixel_t* i, sum, threshold, B; //, t;
        q = (int*)malloc(sizeof(int)*WIDTH*HEIGHT);
    q1 = (int*)malloc(sizeof(int)*WIDTH*HEIGHT);
    q2 = (int*)malloc(sizeof(int)*WIDTH*HEIGHT);
    i = (ia_pixel_t*)malloc(sizeof(ia_pixel_t)*WIDTH*HEIGHT);
    window = 3;
    //stddev = 5;
    s = 0;
    width = WIDTH;
    height = HEIGHT;
threshold = 60;
B = 5;
    //threshold = 80;
    //B = (255) / ((window*window-1)/2);

        for(y = s; y < height; y++)
        {
                for(x = s; x < width; x++)
                {
                        sum = 0;
            for(h = y-(window/2); h < y+(window/2); h++)
                        {
                                for(k = x-(window/2); k < x+(window/2); k++)
                                {
                                        sum += pow(im[h*WIDTH+k], 2);
                                }
                        }
                        sum = sqrt(sum/((window*window)));
            i[y*WIDTH+x] = sum;
//if(sum > 40) printf("%f %f: ", i[y*WIDTH+x], threshold);
            if(sum > threshold)
            {
                q[y*WIDTH+x] = 255;
                //q1[y*WIDTH+x] = 255;
                //q2[y*WIDTH+x] = 255;
            }
            else
            {
                q[y*WIDTH+x] = 0;
                //q1[y*WIDTH+x] = 0;
                //q2[y*WIDTH+x] = 0;
            }
                }
        }
    free(i);
    free(im);
    free(q1);
    free(q2);
    return q;
}

ia_pixel_t *get_gaussian(int ksize)
{
    ia_pixel_t *gaussian = (ia_pixel_t*)malloc(sizeof(ia_pixel_t) * ksize*ksize);
    ia_pixel_t sigma = (ksize/2.0 - 1)*.3 + .8;
    ia_pixel_t sq_sigma = sigma*sigma;
    int i;
    int y = ksize/2;
    int x = ksize/2;
    for(i = 0; i < ksize*ksize; i++)
    {
        y = abs(i/ksize - ksize/2);
        x = abs(i%ksize - ksize/2);
        gaussian[i] = 1/(2*3.1415926*sq_sigma) * exp(-(ia_pixel_t)(y*y + x*x)/(2*sq_sigma));
    }
    return gaussian;
}

ia_pixel_t* get_reflectance_image(IplImage* im, ia_pixel_t* gaussian, int ksize)
{
    int width = WIDTH, height = HEIGHT, s = 0;
    ia_pixel_t *image = (ia_pixel_t*)malloc(WIDTH*HEIGHT*sizeof(ia_pixel_t));
    ia_pixel_t *blur = (ia_pixel_t*)malloc(WIDTH*HEIGHT*sizeof(ia_pixel_t));
        int x, y, tx, ty, h, k;
        ia_pixel_t sum;
    ia_pixel_t lmin = 10000, lmax = 0, max = 255.0, op;
        for(y = 0; y < HEIGHT; y++)
        {
                for(x = 0; x < WIDTH; x++)
                {
                if(((uchar*)(im->imageData + y*im->widthStep))[x] < 1)
                    image[y*WIDTH+x] = 0;
                else
                    //image[y*WIDTH+x] = log((ia_pixel_t)pow(((uchar*)(im->imageData + y*im->widthStep))[x], .4));
                    image[y*WIDTH+x] = log((ia_pixel_t)((uchar*)(im->imageData + y*im->widthStep))[x]);
        }
        }
        for(y = s; y < height; y++)
        {
        for(x = s; x < width; x++)
                {
                        sum = 0;
                        for(h = 0; h < ksize; h++)
                        {
                ty = y+h-ksize/2;
                                for(k = 0; k < ksize; k++)
                                {
                                        tx = x+k-ksize/2;
                    sum += gaussian[h*ksize+k] * image[ty*WIDTH+tx];
                                }
                        }
            blur[y*WIDTH+x] = exp(fabs(sum - image[y*WIDTH+x]));
            if(blur[y*WIDTH+x] > lmax)
                lmax = blur[y*WIDTH+x];
            if(blur[y*WIDTH+x] < lmin)
                lmin = blur[y*WIDTH+x];
        }
    }
    op = (ia_pixel_t)max/(lmax - lmin);
    for(y = s; y < height; y++)
    {
        for(x = s; x < width; x++)
        {
            blur[y*WIDTH+x] = (blur[y*WIDTH+x] - lmin) * op;
            //((uchar*)(im->imageData + y*im->widthStep))[x] = (int)blur[y*WIDTH+x];
        }
    }
    free(image);
    return blur;
}

ia_pixel_t* patch_diff(ia_pixel_t* ima, ia_pixel_t* imb, int ksize)
{
    int i, j, h, k, width, height, s;
    ia_pixel_t* out = (ia_pixel_t*)malloc(sizeof(ia_pixel_t)*WIDTH*HEIGHT), sum;
    width = WIDTH;
    height = HEIGHT;
    s = 0;
    for(i = s; i < height; i++)
    {
        for(j = s; j < width; j++)
        {
            sum = 0;
            for(h = i-ksize/2; h < i+ksize/2; h++)
                for(k = j-ksize/2; k < j+ksize/2; k++)
                    sum += pow(ima[h*WIDTH+k] - imb[h*WIDTH+k], 2);
            out[i*WIDTH+j] = sqrt(sum/ksize);
            if(out[i*WIDTH+j] < 40)
                out[i*WIDTH+j] = 0;
        }
    }
    return out;
}

ia_pixel_t* Iplblur(IplImage* im, ia_pixel_t* gaussian, int ksize)
{
    int y, x, h, k, height = HEIGHT, width = WIDTH, s = 0, tx, ty;
    ia_pixel_t sum, *out = (ia_pixel_t*)malloc(sizeof(ia_pixel_t)*WIDTH*HEIGHT);
        for(y = s; y < height; y++)
        {
                for(x = s; x < width; x++)
                {
                        sum = 0;
                        for(h = 0; h < ksize; h++)
                        {
                                ty = y+h-ksize/2;
                                for(k = 0; k < ksize; k++)
                                {
                                        tx = x+k-ksize/2;
                                        sum += gaussian[h*ksize+k] * ((uchar*)(im->imageData + ty*im->widthStep))[tx];
                                }
                        }
                        out[y*WIDTH+x] = sum;
                }
        }
    return out;
}

void dialate(ia_pixel_t* im, int ksize)
{
    int i, j, h, k, height = HEIGHT, width = WIDTH, s = 0, n;

    for(i = s; i < height; i++)
    {
        for(j = s; j < width; j++)
        {
            n = 0;
            for(h = i-ksize/2; h < i+ksize/2; h++)
            {
                for(k = j-ksize/2; k < j+ksize/2; j++)
                {
                    if(im[h*WIDTH+k] == 255)
                        n++;
                }
            }
            if(n >= 3)
                im[h*WIDTH+k] = 255;
        }
    }
}

**************************************************************************************/

/* these all work */
static inline int offset( int w, int x, int y, int c )
{
    return w*3*y + x*3+c;
}

#define O( y,x ) (s->param->i_width*3*y + x*3)

void draw_best_box( ia_seq_t* s )
{
    int i, j;
    int top, bottom, left, right;
    top    = left  =  INT_MAX;
    bottom = right = -INT_MAX;

    for( i = 0; i < s->param->i_height; i++ )
    {
        for( j = 0; j < s->param->i_width; j++ )
        {
            if( s->iaf->pix[O(i,j)] > 128 )
            {
                top    = (   top > i) ? i : top;
                left   = (  left > j) ? j : left;
                bottom = (bottom < i) ? i : bottom;
                right  = ( right < j) ? j : right;
            }

            s->iar->pix[O(i,j)+0] = 0;
            s->iar->pix[O(i,j)+1] = 0;
            s->iar->pix[O(i,j)+2] = 0;
        }
    }
    
    for( i = top; i <= bottom ; i++ )
    {
        for( j = left; j <= right; j++ )
        {
            s->iar->pix[O(i,j)+0] = 255;
            s->iar->pix[O(i,j)+1] = 255;
            s->iar->pix[O(i,j)+2] = 255;
        }
    }
}

void fstderiv( ia_seq_t* s )
{
    int i, j;
    double dxr, dxg, dxb;
    double dyr, dyg, dyb;
    const double op = 255/sqrt(pow(255*3,2)*2); //  = max / (lmax - lmin)

    for(i = 0; i < s->param->i_height; i++)
    {
        for(j = 0; j < s->param->i_width; j++)
        {
            if( i == 0 || j == 0 || i == s->param->i_height-1 || j == s->param->i_width-1 )
            {
                s->iar->pix[offset(s->param->i_width,j,i,0)] = 0;
                s->iar->pix[offset(s->param->i_width,j,i,1)] = 0;
                s->iar->pix[offset(s->param->i_width,j,i,2)] = 0;
                continue;
            }

            dxr = dxg = dxb =
            dyr = dyg = dyb = 0;

            // left
            dxr -= s->iaf->pix[offset(s->param->i_width,j-1,i-1,0)];
            dxg -= s->iaf->pix[offset(s->param->i_width,j-1,i-1,1)];
            dxb -= s->iaf->pix[offset(s->param->i_width,j-1,i-1,2)];

            dxr -= s->iaf->pix[offset(s->param->i_width,j-1,i+0,0)];
            dxg -= s->iaf->pix[offset(s->param->i_width,j-1,i+0,1)];
            dxb -= s->iaf->pix[offset(s->param->i_width,j-1,i+0,2)];

            dxr -= s->iaf->pix[offset(s->param->i_width,j-1,i+1,0)];
            dxg -= s->iaf->pix[offset(s->param->i_width,j-1,i+1,1)];
            dxb -= s->iaf->pix[offset(s->param->i_width,j-1,i+1,2)];

            // center


            // right
            dxr += s->iaf->pix[offset(s->param->i_width,j+1,i-1,0)];
            dxg += s->iaf->pix[offset(s->param->i_width,j+1,i-1,1)];
            dxb += s->iaf->pix[offset(s->param->i_width,j+1,i-1,2)];

            dxr += s->iaf->pix[offset(s->param->i_width,j+1,i+0,0)];
            dxg += s->iaf->pix[offset(s->param->i_width,j+1,i+0,1)];
            dxb += s->iaf->pix[offset(s->param->i_width,j+1,i+0,2)];

            dxr += s->iaf->pix[offset(s->param->i_width,j+1,i+1,0)];
            dxg += s->iaf->pix[offset(s->param->i_width,j+1,i+1,1)];
            dxb += s->iaf->pix[offset(s->param->i_width,j+1,i+1,2)];

            // top
            dyr += s->iaf->pix[offset(s->param->i_width,j-1,i-1,0)];
            dyg += s->iaf->pix[offset(s->param->i_width,j-1,i-1,1)];
            dyb += s->iaf->pix[offset(s->param->i_width,j-1,i-1,2)];

            dyr += s->iaf->pix[offset(s->param->i_width,j+0,i-1,0)];
            dyg += s->iaf->pix[offset(s->param->i_width,j+0,i-1,1)];
            dyb += s->iaf->pix[offset(s->param->i_width,j+0,i-1,2)];

            dyr += s->iaf->pix[offset(s->param->i_width,j+1,i-1,0)];
            dyg += s->iaf->pix[offset(s->param->i_width,j+1,i-1,1)];
            dyb += s->iaf->pix[offset(s->param->i_width,j+1,i-1,2)];

            // middle


            // bottom
            dyr -= s->iaf->pix[offset(s->param->i_width,j-1,i+1,0)];
            dyg -= s->iaf->pix[offset(s->param->i_width,j-1,i+1,1)];
            dyb -= s->iaf->pix[offset(s->param->i_width,j-1,i+1,2)];

            dyr -= s->iaf->pix[offset(s->param->i_width,j+0,i+1,0)];
            dyg -= s->iaf->pix[offset(s->param->i_width,j+0,i+1,1)];
            dyb -= s->iaf->pix[offset(s->param->i_width,j+0,i+1,2)];

            dyr -= s->iaf->pix[offset(s->param->i_width,j+1,i+1,0)];
            dyg -= s->iaf->pix[offset(s->param->i_width,j+1,i+1,1)];
            dyb -= s->iaf->pix[offset(s->param->i_width,j+1,i+1,2)];

            s->iar->pix[offset(s->param->i_width,j,i,0)] = sqrt(pow(dxr,2)+pow(dyr,2)) * op;
            s->iar->pix[offset(s->param->i_width,j,i,1)] = sqrt(pow(dxg,2)+pow(dyg,2)) * op;
            s->iar->pix[offset(s->param->i_width,j,i,2)] = sqrt(pow(dxb,2)+pow(dyb,2)) * op;
        }
    }
}

void diff( ia_seq_t* s )
{
    int i;

    if( s->i_nrefs == 0 )
    {
        ia_memset( s->iar->pix,0,sizeof(ia_pixel_t)*s->param->i_size*3 );
        return;
    }
    
    for(i = 0; i < s->param->i_size*3; i++)
        s->iar->pix[i] = fabs( s->iaf->pix[i] - s->ref[0]->pix[i] );
}

void bhatta_init ( ia_seq_t* s )
{
    int i, cb, cg;

    /* initialize background model histogram */
    b.bg_hist = (unsigned int****) ia_malloc ( sizeof(unsigned int***)*s->param->i_size );
    for( i = 0; i < s->param->i_size; i++ )
    {
        b.bg_hist[i] = (unsigned int***) ia_malloc ( sizeof(unsigned int**)*s->param->BhattaSettings.NumBins1 );
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
        {
            b.bg_hist[i][cb] = (unsigned int**) ia_malloc ( sizeof(unsigned int*)*s->param->BhattaSettings.NumBins2 );
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
            {
                b.bg_hist[i][cb][cg] = (unsigned int*) ia_malloc ( sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
                ia_memset( b.bg_hist[i][cb][cg],0,sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
            }
        }
    }

    b.pixel_hist = (unsigned int***) ia_malloc ( sizeof(unsigned int**)*s->param->BhattaSettings.NumBins1 );
    for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
    {
        b.pixel_hist[cb] = (unsigned int**) ia_malloc ( sizeof(unsigned int*)*s->param->BhattaSettings.NumBins2 );
        for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
        {
            b.pixel_hist[cb][cg] = (unsigned int*) ia_malloc ( sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
            ia_memset( b.pixel_hist[cb][cg],0,sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
        }
    }

    s->mask = (uint8_t*) ia_malloc( sizeof(uint8_t)*s->param->i_size );
    s->diffImage = (uint8_t*) ia_malloc( sizeof(uint8_t)*s->param->i_size*3 );
}

void bhatta_close ( ia_seq_t* s )
{
    int i, cb, cg;

    ia_free( s->mask );
    ia_free( s->diffImage );

    for( i = 0; i < s->param->i_size; i++ )
    {
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
        {
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
            {
                ia_free( b.bg_hist[i][cb][cg] );
            }
            ia_free( b.bg_hist[i][cb] );
        }
        ia_free( b.bg_hist[i] );
    }
    ia_free( b.bg_hist );

    for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
    {
        for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
        {
            ia_free( b.pixel_hist[cb][cg] );
        }
        ia_free( b.pixel_hist[cb] );
    }
    ia_free( b.pixel_hist );
}

void bhatta_populatebg( ia_seq_t* s )
{
    // Patches are centered around the pixel.  [p.x p.y]-[offset offset] gives the upperleft corner of the patch.
    int offset = s->param->BhattaSettings.SizePatch/2;

    // To reduce divisions.  Used to take a pixel value and calculate the histogram bin it falls in.
    double inv_sizeBins1 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins1);
    double inv_sizeBins2 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins2);
    double inv_sizeBins3 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins3);

    int i = 0, k=0;  // k = current pixel index
    int binB, binG, binR;
    int r,c,ro,co,roi,coi;
    int cg, cb, cr;
    s->param->Settings.bgCount = 0;


    // Histograms are for each pixel by creating create a histogram of the left most pixel in a row.
    // Then the next pixel's histogram in the row is calculated by:
    //   1. removing pixels in the left most col of the previous patch from the histogram and 
    //   2. adding pixels in the right most col of the current pixel's patch to the histogram

    // create temp hist to store working histogram
    unsigned int*** tempHist = (unsigned int***) ia_malloc( sizeof(unsigned int**)*s->param->BhattaSettings.NumBins1 );
    for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
    {
        tempHist[cb] = (unsigned int**) ia_malloc( sizeof(unsigned int*)*s->param->BhattaSettings.NumBins2 );
        for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
        {
            tempHist[cb][cg] = (unsigned int*) ia_malloc( sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
        }
    }

    for( r = 0; r < s->param->i_height; r++ )
    {
        // clear temp patch
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                ia_memset( tempHist[cb][cg],0,sizeof(unsigned int)*s->param->BhattaSettings.NumBins3);

        // create the left most pixel's histogram from scratch
        c = 0;
        int roEnd = r-offset+s->param->BhattaSettings.SizePatch;  // end of patch
        int coEnd = c-offset+s->param->BhattaSettings.SizePatch;  // end of patch
        for( ro = r-offset; ro < roEnd; ro++ )
        {  // cover the row
            roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
            for( co = c-offset; co < coEnd; co++ )
            { // cover the col
                coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));

                // get the pixel loation
                i = (roi*s->param->i_width+coi)*3;
                //figure out whicch bin
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);
                // add to temporary histogram
                tempHist[binB][binG][binR]++;

            }
        }

        // copy temp histogram to left most patch
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                for( cr = 0; cr < s->param->BhattaSettings.NumBins3; cr++ )
                    b.bg_hist[k][cb][cg][cr] += tempHist[cb][cg][cr];

        // increment pixel index
        k++;

        // compute the top row of histograms
        for( c = 1; c < s->param->i_width; c++ )
        {
            // subtract left col
            co = c-offset-1;
            coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
            for( ro = r-offset; ro < r-offset+s->param->BhattaSettings.SizePatch; ro++ )
            {
                roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);

                tempHist[binB][binG][binR]--;
            }

            // add right col
            co = c-offset+s->param->BhattaSettings.SizePatch-1;
            coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
            for( ro = r-offset; ro < r-offset+s->param->BhattaSettings.SizePatch; ro++ )
            {
                roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);
                tempHist[binB][binG][binR]++;
            }

            // copy over            
            for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
                for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                    for( cr = 0; cr < s->param->BhattaSettings.NumBins3; cr++ )
                        b.bg_hist[k][cb][cg][cr] += tempHist[cb][cg][cr];

            // increment pixel index
            k++;
        }
    }
}

int bhatta_estimatefg( ia_seq_t* s )
{
    // Patches are centered around the pixel.  [p.x p.y]-[offset offset] gives the upperleft corner of the patch.              
    int offset = s->param->BhattaSettings.SizePatch/2;
    // To reduce divisions.  Used to take a pixel value and calculate the histogram bin it falls in.
    double inv_sizeBins1 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins1);
    double inv_sizeBins2 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins2);
    double inv_sizeBins3 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins3);
    double inv_nPixels = 1.f/((double) (s->param->BhattaSettings.SizePatch*s->param->BhattaSettings.SizePatch*s->param->BhattaSettings.SizePatch*s->param->BhattaSettings.SizePatch*s->param->Settings.NumBgFrames));

    int binB, binG, binR;
    int i = 0, cb, cg, cr;
    double diff = 0;
    // for each pixel
    int coi=0,roi=0,r=0,c=0,co=0,ro=0,pIndex=0;
    int pos = 0;

    // clear s->mask/diff image
    ia_memset( s->mask,0,s->param->i_size*sizeof(uint8_t) );
    ia_memset( s->diffImage,0,s->param->i_size*3*sizeof(uint8_t) );


    // as in the populateBg(), we compute the histogram by subtracting/adding cols

    for( r = 0; r < s->param->i_height; r++ )
    {
        c=0;
        pIndex = r*s->param->i_width+c;

        // for the first pixel

        // clear patch histogram
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                ia_memset( b.pixel_hist[cb][cg],0,s->param->BhattaSettings.NumBins3*sizeof(unsigned int) );

        // compute patch
        int roEnd = r-offset+s->param->BhattaSettings.SizePatch;
        int coEnd = c-offset+s->param->BhattaSettings.SizePatch;
        for( ro = r-offset; ro < roEnd; ro++ )
        {
            roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
            for( co = c-offset; co < coEnd; co++ )
            {
                coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);
                b.pixel_hist[binB][binG][binR]++;
            }
        }

        // compare histograms
        diff = 0;
        int temp1 = 0;
        int temp2 = 0;
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
        {
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
            {
                for( cr = 0; cr < s->param->BhattaSettings.NumBins3; cr++ )
                {
                    diff += sqrt((double)b.pixel_hist[cb][cg][cr] * (double)b.bg_hist[pIndex][cb][cg][cr] * inv_nPixels);
                    temp1 += b.pixel_hist[cb][cg][cr];
                    temp2 += b.bg_hist[pIndex][cb][cg][cr];
                }
            }
        }

        // renormalize diff so that 255 = very diff, 0 = same
        // create result images
        s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
        s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
        s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
        s->mask[pIndex] = (s->diffImage[pos-1] > s->param->Settings.Threshold ? 255 : 0);


        // iterate through the rest of the row
        for( c = 1; c < s->param->i_width; c++ )
        {
            pIndex = r*s->param->i_width+c;

            //remove left col
            co = c-offset-1;
            coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
            for( ro = r-offset; ro < r-offset+s->param->BhattaSettings.SizePatch; ro++ )
            {
                roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);

                b.pixel_hist[binB][binG][binR]--;
            }

            // add right col
            co = c-offset+s->param->BhattaSettings.SizePatch-1;
            coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
            for( ro = r-offset; ro < r-offset+s->param->BhattaSettings.SizePatch; ro++ )
            {
                roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);
                b.pixel_hist[binB][binG][binR]++;
            }

            // compare histograms
            diff = 0;
            temp1 = 0;
            temp2 = 0;
            for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
            {
                for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                {
                    for( cr = 0; cr < s->param->BhattaSettings.NumBins3; cr++ )
                    {
                        diff += sqrt((double)b.pixel_hist[cb][cg][cr] * (double)b.bg_hist[pIndex][cb][cg][cr] * inv_nPixels);
                        temp1 += b.pixel_hist[cb][cg][cr];
                        temp2 += b.bg_hist[pIndex][cb][cg][cr];
                    }
                }
            }

            // create result images     
            s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
            s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
            s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
            s->mask[pIndex] = (s->diffImage[pos-1] > s->param->Settings.Threshold ? 255 : 0);
        }
    }
    
    ia_memcpy_uint8_to_pixel( s->iar->pix,s->diffImage,s->param->i_size*3 );
//    for( i = 0; i < s->param->i_size*3; i++ )
//        s->iar->pix[i] = s->diffImage[i];
    return 0;
}

int bhatta( ia_seq_t* s )
{
    if( (int)s->i_frame < s->param->Settings.NumBgFrames)
    {
        bhatta_populatebg( s );
    }
    else
    {
        if( bhatta_estimatefg(s) )
            return 1;
/*
        p->Settings.bgCount++;
        if( p->Settings.bgCount == p->Settings.BgStep)
        {
            if( b.BhattaOptSettings.useBgUpdateMask )
                bhatta_updatebg( iaf,diff,mask );
            else
                bhatta_updatebg( iaf,diff,NULL );
            p->Settings.bgCount = 0;
        }
*/
    }
    return 0;
}

static inline void copy( ia_seq_t* s )
{
    ia_memcpy_pixel( s->iar->pix,s->iaf->pix,s->param->i_size*3 );
}

static inline void flow( ia_seq_t* s )
{
    int i, j, h, k;
    double lmin, op;
    double dzr, dzg, dzb;

    if( s->i_nrefs < 2 )
    {
        ia_memset( s->iar->pix,0,sizeof(ia_pixel_t)*s->param->i_size*3 );
        return;
    }

    lmin = -255*9;
    op = 255.0 / (255*9*2);  //  = max / (lmax - lmin)

    for ( i = 0; i < s->param->i_height; i++ )
    {
        for ( j = 0; j < s->param->i_width; j++)
        {

            if ( i == 0 || j == 0 || j == s->param->i_width-1 || i == s->param->i_height-1 )
            {
                s->iar->pix[offset(s->param->i_width,j,i,0)] =
                s->iar->pix[offset(s->param->i_width,j,i,1)] =
                s->iar->pix[offset(s->param->i_width,j,i,2)] = 0;
                continue;
            }

            dzr = dzg = dzb = 0;
            for( h = i-1; h < i+1; h++ )
            {
                for( k = j-1; k < j+1; k++ )
                {
                    dzr += (s->iaf->pix[offset(s->param->i_width,k,h,0)]
                         - s->ref[1]->pix[offset(s->param->i_width,k,h,0)]);
                    dzg += (s->iaf->pix[offset(s->param->i_width,k,h,1)]
                         - s->ref[1]->pix[offset(s->param->i_width,k,h,1)]);
                    dzb += (s->iaf->pix[offset(s->param->i_width,k,h,2)]
                         - s->ref[1]->pix[offset(s->param->i_width,k,h,2)]);
                }
            }

            s->iar->pix[offset(s->param->i_width,j,i,0)] = ( dzr - lmin ) * op;
            s->iar->pix[offset(s->param->i_width,j,i,1)] = ( dzg - lmin ) * op;
            s->iar->pix[offset(s->param->i_width,j,i,2)] = ( dzb - lmin ) * op;
        }
    }
}

#define IA_PRINT( s )\
{\
    if( p->b_verbose )\
        printf( s );\
}

#define THREADED 0

static inline ia_seq_t* analyze_init( ia_param_t* p )
{
    int j;
    ia_seq_t* ias = ia_seq_open( p );

    if( ias == NULL )
        return NULL;

    /* call any init functions */
    for ( j = 0; p->filter[j] != 0; j++ )
    {
        switch ( p->filter[j] )
        {
            case COPY:
                break;
            case DERIV:
                break;
            case BHATTA:
                bhatta_init( ias );
                break;
            case DIFF:
                break;
            default:
                break;
        }
    }
    return ias;
}

static inline void analyze_deinit( ia_seq_t* s )
{
    int j;

    /* call any filter specific close functions */
    for ( j = 0; s->param->filter[j] != 0; j++ )
    {
        switch ( s->param->filter[j] )
        {
            case COPY:
                break;
            case DERIV:
                break;
            case BHATTA:
                bhatta_close( s );
                break;
            case DIFF:
                break;
            default:
                break;
        }
    }

    ia_seq_close( s );
}

int analyze( ia_param_t* p )
{
    int j, wt_status;
    pthread_t write_thread;

    ia_seq_t* ias = analyze_init( p );
    if( ias == NULL )
        return 1;

    wt_status = 0;
    while( ( !ia_seq_getimage(ias) ) )
    {
        // process frame with selected filters
        for ( j = 0; p->filter[j] != 0; j++ )
        {
            switch ( p->filter[j] )
            {
                case COPY:
                    IA_PRINT( "doing copy...\t" );
                    copy( ias );
                    break;
                case DERIV:
                    IA_PRINT( "doing deriv...\t" );
                    fstderiv( ias );
                    break;
                case BHATTA:
                    IA_PRINT( "doing bhatta...\n" );
                    if( bhatta(ias) )
                        return 1;
                    break;
                case MBOX:
                    IA_PRINT( "doing best bounding box...\n" );
                    draw_best_box( ias );
                    break;
                case DIFF:
                    IA_PRINT( "doing diff...\n" );
                    diff( ias );
                    break;
                case FLOW:
                    IA_PRINT( "doing flow...\n" );
                    flow( ias );
                default:
                    break;
            }
        }
        if( p->b_verbose )
            printf( "captured a frame: %lld\n",ias->i_frame );

        /* this threaded stuff was a little buggy */
        if( THREADED )
        {
            if( wt_status && p->output_directory[0] != 0 )
            {
                pthread_join( write_thread,NULL );
                wt_status = 0;
            }
            if( p->output_directory[0] != 0 )
            {
                if( pthread_create(&write_thread,NULL,ia_seq_saveimage,(void*)ias->iar) != 0 )
                    return 1;
                else
                    wt_status = 1;
            }
        }
        else
        {
            if( p->output_directory[0] != 0 )
                ia_seq_saveimage( ias );
        }

        if( ias->i_frame > 500 )
            return 0;
    }

    if( THREADED )
        if( wt_status && p->output_directory[0] != 0 )
            pthread_join( write_thread,NULL );

    analyze_deinit( ias );

    return 0;
}
