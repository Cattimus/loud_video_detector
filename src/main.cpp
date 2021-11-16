#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>

#include <string>
#include <vector>

#include "wav_16.h"
#include "audio_processing.h"
using namespace std;

void fix_header(vector<uint8_t> *data, uint32_t total_bytes);
void handle_flags(int argc, char *argv[]);
void handle_conditional_flag(const char *flag);
void handle_argument_flag(const char *flag, const char *argument);
void print_help();

string path = "";

double peak_threshold = -8;
double average_threshold = -10;
double sudden_threshold = 20;
int time_window = 200;

bool peak_enabled = true;
bool average_enabled = true;
bool sudden_enabled = true;
bool text_enabled = true;
bool boolean_output = false;
bool debug_text = false;

int main(int argc, char *argv[])
{
	struct timespec program_start;
	clock_gettime(CLOCK_MONOTONIC, &program_start);
	
	if (argc == 1)
	{
		print_help();
		exit(1);
	}
	
	//ingest command line args
	if (argc > 2)
	{
		handle_flags(argc, argv);
	}
	
	if (path == "")
	{
		fprintf(stderr, "No input path or url provided for program. Use the -i command line argument to pass an input path/url.\n");
		exit(1);
	}
	
	string command;
	command.append("ffmpeg -i ");
	command.append(path);
	command.append(" -f wav -loglevel quiet -");
	
	FILE *command_output = popen(command.c_str(), "r");
	uint32_t total_bytes = 0;
	
	//use ffmpeg to download audio from source
	vector<uint8_t> data;
	if (command_output != NULL)
	{
		uint8_t buffer[8192];
		memset(buffer, 0, 8192);
		int len = 0;
		
		while (((len = fread(buffer, 1, 8192, command_output)) > 0))
		{
			data.insert(data.end(), buffer, buffer + len);
			memset(buffer, 0, 8192);
			total_bytes += len;
		}
		
		pclose(command_output);
		
		//if this activates, ffmpeg couldn't find the file or failed to convert the file
		if (total_bytes < 44)
		{
			fprintf(stderr, "File not found for ffmpeg failed conversion.\n");
			exit(1);
		}
		
		fix_header(&data, total_bytes);
	}
	
	wav_16 input_wav = wav_16(data);
	
	//return values for comparison functions
	int peak_val = 0;
	int average_val = 0;
	int sudden_val = 0;
	
	//calculate peak loudness values and compare with threshold
	if(peak_enabled)
	{
		struct timespec end;
		struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);
		peak_val = check_peak(&input_wav, time_window, peak_threshold);
		clock_gettime(CLOCK_MONOTONIC, &end);
		
		if(debug_text)
		{
			printf("peak analysis execution time: %.2fms\n", (double)((end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec)) / 1000000);
		}
	}
	
	//calculate average volume and compare with threshold
	if(average_enabled)
	{
		struct timespec end;
		struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);
		average_val = check_average(&input_wav, sudden_threshold);
		clock_gettime(CLOCK_MONOTONIC, &end);
		
		if(debug_text)
		{
			printf("average analysis execution time: %.2fms\n", (double)((end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec)) / 1000000);
		}
		
	}
	
	//calculate sudden volume rises and compare with threshold
	if(sudden_enabled)
	{
		struct timespec end;
		struct timespec start;
		clock_gettime(CLOCK_MONOTONIC, &start);
		sudden_val = check_sudden(&input_wav, time_window, sudden_threshold);
		clock_gettime(CLOCK_MONOTONIC, &end);
		
		if(debug_text)
		{
			printf("sudden analysis execution time: %.2fms\n", (double)((end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec)) / 1000000);
		}
	}
	
	if(debug_text)
	{
		printf("\n");
	}
	
	//print results
	if(boolean_output)
	{
		if(peak_val != 0 || sudden_val != 0 || average_val != 0)
		{
			printf("1\n");
		}
		else
		{
			printf("0\n");
		}
	}
	else
	{
		if(peak_enabled)
		printf("Number of peaks above threshold: %d\n", peak_val);
		if(average_enabled)
		printf("Average volume exceeds threshold? %s\n", average_val ? "yes" : "no");
		if(sudden_enabled)
		printf("Number of sudden volume rises: %d\n", sudden_val);
	}
	
	if (debug_text)
	{
		printf("\n");
		printf("peak threshold: %.2f\n", peak_threshold);
		printf("average threshold: %.2f\n", average_threshold);
		printf("sudden threshold: %.2f\n", sudden_threshold);
		printf("window duration: %d\n\n", time_window);
		
		printf("peak enabled? ");
		printf(peak_enabled ? "true\n" : "false\n");
		printf("average enabled? ");
		printf(average_enabled ? "true\n" : "false\n");
		printf("sudden enabled? ");
		printf(sudden_enabled ? "true\n" : "false\n");
		
		struct timespec program_end;
		clock_gettime(CLOCK_MONOTONIC, &program_end);
		printf("\n\n");
		printf("total program execution time: %.2fms\n", (double)((program_end.tv_sec - program_start.tv_sec) * 1000000000 + (program_end.tv_nsec - program_start.tv_nsec)) / 1000000);
	}
	return 0;
}

//replace default (0xFFFF) with proper value
void fix_header(vector<uint8_t> *data, uint32_t total_bytes)
{
	//fix data provided by ffmpeg (chunk sizes)
	uint32_t temp = total_bytes - 8;
	memcpy(data->data() + 4, &temp, 4);
	
	uint32_t current_index = 12;
	uint32_t tag = 0;
	uint32_t size = 0;
	
	//seek through chunks for data chunk
	while (strncmp((const char *)&tag, "data", 4) != 0)
	{
		current_index += size;
		memcpy(&tag, data->data() + current_index, 4);
		current_index += 4;
		memcpy(&size, data->data() + current_index, 4);
		current_index += 4;
	}
	
	//write proper size to data chunk
	current_index -= 4;
	temp = total_bytes - current_index - 4;
	memcpy(data->data() + current_index, &temp, 4);
}

//extract values from flags before main is executed
void handle_flags(int argc, char *argv[])
{
	int arguments = argc - 1;
	int index = 1;
	
	while (arguments > 0)
	{
		if (strlen(argv[index]) >= 2)
		{
			if (strncmp(argv[index], "--", 2) == 0)
			{
				handle_conditional_flag(argv[index]);
				arguments--;
				index++;
			}
			else if (argv[index][0] == '-')
			{
				if (arguments >= 2)
				{
					handle_argument_flag(argv[index], argv[index + 1]);
					arguments -= 2;
					index += 2;
				}
				else
				{
					fprintf(stderr, "no argument provided to flag %s\n", argv[index]);
					exit(1);
				}
			}
			else
			{
				fprintf(stderr, "Unrecognized argument: %s\n", argv[index]);
				exit(1);
			}
		}
	}
}

//--flag
void handle_conditional_flag(const char *flag)
{
	int len = strlen(flag);
	
	if (strncmp(flag, "--disable-peak", len) == 0)
	{
		peak_enabled = false;
	}
	else if (strncmp(flag, "--disable-average", len) == 0)
	{
		average_enabled = false;
	}
	else if (strncmp(flag, "--disable-sudden", len) == 0)
	{
		sudden_enabled = false;
	}
	else if (strncmp(flag, "--boolean-output", len) == 0)
	{
		boolean_output = true;
	}
	else if (strncmp(flag, "--disable-text", len) == 0)
	{
		text_enabled = false;
	}
	else if (strncmp(flag, "--debug-text", len) == 0)
	{
		debug_text = true;
	}
	else
	{
		fprintf(stderr, "Unrecognized argument: %s\n", flag);
		exit(1);
	}
}

//-flag argument
void handle_argument_flag(const char *flag, const char *argument)
{
	int flag_len = strlen(flag);
	int arg_len = strlen(argument);
	
	bool arg_valid = false;
	
	//attempt to convert argument
	double arg_val = atof(argument);
	
	if (strncmp(flag, "-peak", flag_len) == 0)
	{
		peak_threshold = arg_val;
	}
	else if (strncmp(flag, "-average", flag_len) == 0)
	{
		average_threshold = arg_val;
	}
	else if (strncmp(flag, "-sudden", flag_len) == 0)
	{
		sudden_threshold = arg_val;
	}
	else if (strncmp(flag, "-window", flag_len) == 0)
	{
		time_window = (int)arg_val;
		
		if (time_window == 0)
		{
			fprintf(stderr, "Invalid time window of 0ms was provided.\n");
			exit(1);
		}
	}
	else if (strncmp(flag, "-i", flag_len) == 0)
	{
		path = argument;
	}
	else
	{
		fprintf(stderr, "Unrecognized argument: %s\n", flag);
		exit(1);
	}
}

void print_help()
{
	string help_message;
	help_message.append("Usage: loud_detector [url or path of file] [flags]\n\n");
	
	help_message.append("[FLAGS]:\n");
	help_message.append("--disable-peak: Disables peak volume detection.\n");
	help_message.append("--disable-average: Disables average volume check.\n");
	help_message.append("--disable-sudden: Disables sudden volume spike detection.\n");
	help_message.append("--disable-text: Displays only return values without extra text.\n");
	help_message.append("--boolean-output: Returns 1(true) or 0(false) with no extra values.\n");
	help_message.append("--debug-text: Enable text display of internal values for debugging/troubleshooting purposes.\n\n");
	
	help_message.append("[ARGUMENT FLAGS]:\n");
	help_message.append("-i [input path or url]: set the input for the program. (default is null).\n");
	help_message.append("-peak [volume]: sets the volume at which peaks are detected as loud to [volume] db. (default is -8).\n");
	help_message.append("-average [volume]: sets the threshold at which the average audio is detected as loud. (default is -10).\n");
	help_message.append("-sudden [volume]: sets the threshold at which sudden increases in volume are detected. (default is 20).\n");
	help_message.append("-window [time in ms]: sets the time window for each audio sample. (default 200).\n");
	
	help_message.append("[INFO]:\n");
	help_message.append("peak detection: if any window exceeds the threshold db, it will be flagged.\n\n");
	help_message.append("average detection: if the average volume exceeds the threshold db, it will be flagged.\n\n");
	help_message.append("sudden detection: sudden rises in volume will be flagged, if there is a difference of [threshold]db within 5 time windows, it will be flagged.");
	help_message.append(" somewhat prone to error, try disabling if you're getting too many false positives.\n");
	
	fprintf(stderr, "%s\n", help_message.c_str());
}
