/******************************************************************************
 * Copyright (c) 2008 Joey Degges
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *****************************************************************************/

#include "grayscale.h"

inline void grayscale_exec( ia_seq_t* s, ia_filter_param_t* fp, ia_image_t** iaim, ia_image_t* iar )
{
    ia_image_t* iaf = iaim[0];
    int i;

    for( i = s->param->i_height; i--; )
    {
        int j;
        for( j = s->param->i_width; j--; )
        {
            int pix;
            double gray = 0.30 * iaf->pix[offset(iaf->i_pitch,j,i,0)]
                        + 0.59 * iaf->pix[offset(iaf->i_pitch,j,i,1)]
                        + 0.11 * iaf->pix[offset(iaf->i_pitch,j,i,2)];
            for( pix = 3; pix--; )
                iar->pix[offset(iaf->i_pitch,j,i,pix)] = gray;
        }
    }
    fp = fp;
}
