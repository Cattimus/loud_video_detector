#include "audio_processing.h"

db_channels calculate_db(wav_16 *audio, int start_sample, int num_samples)
{
	uint64_t total_l = 0;
	uint64_t total_r = 0;
	
	db_channels to_return;
	
	//calculate mean square
	for (int i = 0; i < num_samples; i++)
	{
		uint8_t *current_loc = audio->get_sample(start_sample + i);
		
		int16_t r = 0;
		int16_t l = 0;
		
		l |= *current_loc;
		l |= (*(current_loc + 1) << 8);
		
		r |= *(current_loc + 2);
		r |= (*(current_loc + 3) << 8);
		
		total_r += pow(r, 2);
		total_l += pow(l, 2);
	}
	
	//calculate root mean square
	double RMS_l = sqrt(total_l / (double)num_samples);
	double RMS_r = sqrt(total_r / (double)num_samples);
	
	//use RMS values to calculate the average DB of each channel
	to_return.l = 20 * log10(RMS_l / (double)32768) + 3.0103;
	to_return.r = 20 * log10(RMS_r / (double)32768) + 3.0103;
	
	return to_return;
}

//calculate the number of times peak volume has exceeded threshold
int check_peak(wav_16 *audio, int window_duration_ms, double db_threshold)
{
	int samples_per_window = audio->get_window(window_duration_ms);
	int total_samples = audio->total_samples();
	int total_windows = total_samples / samples_per_window;
	
	int over_counter = 0;
	
	for (int i = 0; i < total_windows; i++)
	{
		db_channels result = calculate_db(audio, i * samples_per_window, samples_per_window);
		
		if (result.l >= db_threshold || result.r >= db_threshold)
		{
			over_counter += 1;
		}
	}
	
	return over_counter;
}

/* check for sudden increases in volume across the audio file.
db_threshold represents the volume change at which to flag the audio.
ex: a volume change of 20db in less than 5 windows */
int check_sudden(wav_16 *audio, int window_duration_ms, double db_threshold)
{
	int samples_per_window = audio->get_window(window_duration_ms);
	int total_samples = audio->total_samples();
	int total_windows = total_samples / samples_per_window;
	
	int over_counter = 0;
	
	vector<db_channels> results;
	
	for (int i = 0; i < total_windows; ++i)
	{
		db_channels result = calculate_db(audio, i * samples_per_window, samples_per_window);
		results.push_back(result);
	}
	
	for (int i = 0; i < results.size(); ++i)
	{
		for (int x = 1; x < 6 && ((i + x) < results.size()); x++)
		{
			int z = i + x;
			//indicates volume is rising
			if ((results[z].l >= results[i].l) || (results[z].r >= results[i].r))
			{
				//since we know these values are almost always negative, we can get away with fewer calls to abs()
				//the edge case of results[x] being at 0db is irrelevant because it's already at maximum volume
				double ldiff = abs(results[z].l - results[i].l);
				double rdiff = abs(results[z].r - results[i].r);
				
				if (ldiff >= db_threshold || rdiff >= db_threshold)
				{
					over_counter++;
					
					//place new seek location after peak volume and continue
					i += x + 1;
					break;
				}
			}
		}
	}
	
	return over_counter;
}

//calculate the average db of the audio file and compare it against the threshold provided
int check_average(wav_16 *audio, double db_threshold)
{
	db_channels result = calculate_db(audio, 0, audio->total_samples());
	
	if (result.l >= db_threshold || result.r >= db_threshold)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
