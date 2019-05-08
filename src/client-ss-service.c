/*
 ============================================================================
 Name        : client-ss-service.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#define PERIOD_SIZE 800
#define BUF_SIZE (PERIOD_SIZE * 2)

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <zmq.h>
#include <json-c/json.h>

#define REQ_CONNECT	1
#define REP_ACCEPT	1

#define RECV_BUF_LEN 4096
#define randof(num)  (int) ((float) (num) * random () / (RAND_MAX + 1.0))

int setparams(snd_pcm_t *handle, char *name) {
	snd_pcm_hw_params_t *hw_params;
	int err;

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
				snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_any(handle, hw_params)) < 0) {
		fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
				snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_access(handle, hw_params,
			SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params_set_format(handle, hw_params,
			SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
		exit(1);
	}

	unsigned int rate = 16000;
	if ((err = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, 0))
			< 0) {
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
		exit(1);
	}
	//printf("Rate for %s is %d\n", name, rate);

	if ((err = snd_pcm_hw_params_set_channels(handle, hw_params, 1)) < 0) {
		fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
		exit(1);
	}

	snd_pcm_uframes_t buffersize = BUF_SIZE;
	if ((err = snd_pcm_hw_params_set_buffer_size_near(handle, hw_params,
			&buffersize)) < 0) {
		printf("Unable to set buffer size %li: %s\n", (long int) BUF_SIZE,
				snd_strerror(err));
		exit(1);
		;
	}

	snd_pcm_uframes_t periodsize = PERIOD_SIZE;
	//fprintf(stderr, "period size now %d\n", (int) periodsize);
	if ((err = snd_pcm_hw_params_set_period_size_near(handle, hw_params,
			&periodsize, 0)) < 0) {
		printf("Unable to set period size %li: %s\n", periodsize,
				snd_strerror(err));
		exit(1);
	}

	if ((err = snd_pcm_hw_params(handle, hw_params)) < 0) {
		fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
		exit(1);
	}

	snd_pcm_uframes_t p_psize;
	snd_pcm_hw_params_get_period_size(hw_params, &p_psize, NULL);
	//fprintf(stderr, "period size %d\n", (int) p_psize);

	snd_pcm_hw_params_get_buffer_size(hw_params, &p_psize);
	//fprintf(stderr, "buffer size %d\n", (int) p_psize);
	unsigned int val;

	snd_pcm_hw_params_get_period_time(hw_params, &val, NULL);

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(handle)) < 0) {
		fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
				snd_strerror(err));
		exit(1);
	}

	return val;
}

int main(int argc, char *argv[]) {

	int err;
	int seed;
	int rc;

	snd_pcm_t *capture_handle;

	unsigned int val;

	char buf[BUF_SIZE * 2];
	char worker_tcp[256];
	char worker_identity[32];
	char recv_buff[RECV_BUF_LEN];

	///Check for command line arguments
	if (argc != 4) {
		fprintf(stderr,
				"Usage: %s <url> <input-soundCard>  <output-soundCard> \n",
				argv[0]);
		fprintf(stderr, "Ex: %s 10.226.174.94:1992 default default \n",
				argv[0]);
		exit(1);
	}

	void *context = zmq_ctx_new();
	void *stream_frontend = zmq_socket(context, ZMQ_DEALER);

	/*create identity worker identification*/
	seed = time(NULL);
	srand(seed * getpid());
	sprintf(worker_identity, "%04X-%04X", randof(0x10000), randof(0x10000));
	zmq_setsockopt(stream_frontend, ZMQ_IDENTITY, worker_identity,
			strlen(worker_identity));

	/* connection to engine identification*/
	sprintf(worker_tcp, "tcp://%s", argv[1]);
	//printf("%s\n", worker_tcp);
	rc = zmq_connect(stream_frontend, worker_tcp);
	assert(rc == 0);

	/*********** In card **********/
	if ((err = snd_pcm_open(&capture_handle, argv[2], SND_PCM_STREAM_CAPTURE, 0))
			< 0) {
		fprintf(stderr, "cannot open audio device %s (%s)\n", argv[2],
				snd_strerror(err));
		exit(1);
	}

	val = setparams(capture_handle, "capture");
	assert(val != 0);

	/************* Capture and Play Voice ***************/
	int nread;

	// Connect To Engine
	strcpy(buf, "connect-smartspeaker");
	zmq_send(stream_frontend, buf, strlen(buf), 0);
	zmq_pollitem_t items[] = { { stream_frontend, 0, ZMQ_POLLIN, 0 } };
	rc = zmq_poll(items, 1, 10);
	if (rc == -1) {
		return 1;
	}

	if (items[0].revents & ZMQ_POLLIN) {
		memset(recv_buff, 0, RECV_BUF_LEN);
		rc = zmq_recv(stream_frontend, recv_buff, RECV_BUF_LEN, 0);
		if (rc > 0) {
			if (recv_buff[0] != REP_ACCEPT) {
				return 1;
			}
		}
	}

	struct json_object *jmessage;
	struct json_object *jlist;
	struct json_object *jdata;
	struct json_object *jpath;
	char res_path[2048];

	//Streaming To Engine
	printf("Streaming Ready...\n");
	while (1) {

		//printf("record...\n");
		snd_pcm_prepare(capture_handle);
		memset(buf, 0, BUF_SIZE * 2);
		if ((nread = snd_pcm_readi(capture_handle, buf, BUF_SIZE)) != BUF_SIZE) {
			if (nread < 0) {
				fprintf(stderr, "read from audio interface failed (%s)\n",
						snd_strerror(nread));
			} else {
				fprintf(stderr,
						"read from audio interface failed after %d frames\n",
						nread);
			}
			snd_pcm_prepare(capture_handle);
			continue;
		}

		zmq_send(stream_frontend, buf, BUF_SIZE * 2, 0);

		zmq_pollitem_t items[] = { { stream_frontend, 0, ZMQ_POLLIN, 0 } };

		// poll every 10 milliseconds
		rc = zmq_poll(items, 1, 10);
		if (rc == -1) {
			break;              //  Interrupted
		}

		if (items[0].revents & ZMQ_POLLIN) {
			memset(recv_buff, 0, RECV_BUF_LEN);
			rc = zmq_recv(stream_frontend, recv_buff, RECV_BUF_LEN, 0);
			recv_buff[rc] = '\0';

			jmessage = json_tokener_parse(recv_buff);

			//printf("jmessage = %s \n", json_object_to_json_string(jmessage));

			if (jmessage != NULL) {
				break;

			}

		}
	}

	json_object_object_get_ex(jmessage, "list", &jlist);

	if (jlist != NULL) {
		int i = 0;
		system("mpc stop");
		system("mpc clear");

		while (i < json_object_array_length(jlist)) {

			jdata = json_object_array_get_idx(jlist, i);
			json_object_object_get_ex(jdata, "path", &jpath);
			if (jpath != NULL) {
				printf("PATH %d: %s \n", i, json_object_get_string(jpath));

				sprintf(res_path, "mpc add %s", json_object_get_string(jpath));
				system(res_path);
				i++;
			}
		}
	}
	system("mpc play");
	json_object_put(jmessage);

	snd_pcm_drain(capture_handle);
	snd_pcm_close(capture_handle);
	zmq_close(stream_frontend);
	zmq_ctx_destroy(context);

	return 0;
}
