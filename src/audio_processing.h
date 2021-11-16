#pragma once

#include <math.h>
#include <stdlib.h>

#include <vector>

#include "wav_16.h"


typedef struct DBchannels
{
	double l;
	double r;
}db_channels;

db_channels calculate_db(wav_16* audio, int start_sample, int num_samples);
int check_peak(wav_16* audio, int window_duration_ms, double db_threshold);
int check_sudden(wav_16* audio, int window_duration_ms, double db_threshold);
int check_average(wav_16* audio, double db_threshold);