#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <fsp/dsp.h>

const int DEBUG_PRINT = 0;

int test_mode(float* out, int nsamples)
{
  int max_loc = 0;
  double max = 0.0f;
  for (int i = 0; i < nsamples; i++) {
    if (out[i] > max) {
      max = out[i];
      max_loc = i;
    }
  }
  return max_loc;
}

double test_mean(float* out, int nsamples)
{
  double sum = 0.0f;
  for (int i = 0; i < nsamples; i++)
    sum += out[i] * i;

  return sum;
}

double test_integral(float* out, int nsamples)
{
  double sum = 0.0f;
  for (int i = 0; i < nsamples; i++) {
    sum += out[i];
  }
  return sum;
}

double gaussian_std_dev(int width)
{
  return sqrt( (width * width - 1.0) / 3.0);
}

void test_sma_4(int nsamples, int width, int delta_loc, float delta_amp)
{
  float* in = calloc(nsamples, sizeof(float));
  in[delta_loc] = delta_amp;

  float* out = calloc(nsamples, sizeof(float));
  const int workspace_size = nsamples + width * 4 + 2;
  float* workspace = calloc(workspace_size, sizeof(float));

  int start = 0;
  int stop = nsamples;
  float gain = 1.0;
  float offset = 0.0;

  int delay =  sma_4(in, out, workspace, nsamples, width, &start, &stop, &gain, &offset);

  double integral = test_integral(out, nsamples);
  double mean = test_mean(out, nsamples) / integral;
  int mode = test_mode(out, nsamples);

  if (DEBUG_PRINT) {
    fprintf(stderr, "%d = sma_4(..,..,..,nsamples=%d,width=%d,std=%f,start=%d,stop=%d,gain=%g,offset=%g) delta_loc=%d; mean=%f mode=%d integral=%f\n",
      delay, nsamples, width, gaussian_std_dev(width), start, stop, gain, offset, delta_loc, mean, mode, integral
    );
    fprintf(stderr, "mode values: %e %e %e\n", out[mode-1], out[mode], out[mode+1]);
  }

  const double epsilon = 1e-6 * delta_amp;
  assert(delay == start);
  assert(delay == (nsamples - stop));
  assert(mode == delta_loc);
  assert(round(mean) == delta_loc);
  assert(fabs(test_integral(out, nsamples) - test_integral(in, nsamples)) < epsilon);

  free(workspace);
  free(out);
  free(in);
}

int main(void)
{
  for (int width = 1; width <= 2500; width++) {
    int presum_length = 4 * width - 4;
    int nsamples = presum_length * 2 + 3;
    int delta_loc = nsamples / 2;
    test_sma_4(nsamples, width, delta_loc, 1.0f);
    test_sma_4(nsamples, width, delta_loc, FLT_MAX / ((double)width*width*width*width));
  }

  return 0;
}
