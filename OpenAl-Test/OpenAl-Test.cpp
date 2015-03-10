#include <AL\al.h>
#include <AL\alc.h>
#include <AL\alext.h>
#include <string>
#include <assert.h>
#include <Windows.h>

#define WANT_THREADING 1
#define NUM_BUFFERS 4

class OpenAlBackend
{
public:

	ALuint buffers[NUM_BUFFERS];
	ALuint source;

	OpenAlBackend::~OpenAlBackend()
	{
		Cleanup();
	}

	void Initialize()
	{
		ALCdevice *device = alcOpenDevice(nullptr);

		if (!device)
		{
			printf_s("Couldn't open a device\n");
			assert(false);
		}

		ALCcontext *context = alcCreateContext(device, nullptr);

		if (!context)
		{
			alcCloseDevice(device);
			printf_s("Couldn't set a context\n");
			assert(false);
		}

		if (alcMakeContextCurrent(context) == ALC_FALSE)
		{
			alcDestroyContext(context);
			alcCloseDevice(device);
			printf_s("Couldn't make context current\n");
			assert(false);
		}

		printf_s("Opened %s\n", alcGetString(device, ALC_DEVICE_SPECIFIER));
	}

	void CreateBuffersAndSources()
	{
		alGenBuffers(NUM_BUFFERS, buffers);

		ALenum error = alGetError();
		if (error != AL_NO_ERROR)
		{
			printf_s("Couldn't create buffers\n");
			printf_s("%s", alGetString(error));
			assert(false);
		}

		alGenSources(1, &source);
		error = alGetError();
		if (error != AL_NO_ERROR)
		{
			printf_s("Couldn't create source object\n");
			printf_s("%s", alGetString(error));
			assert(false);
		}
	}

	void Start()
	{
		alSourceRewind(source);
		alSourcei(source, AL_BUFFER, 0);

		for (int i = 0; i < NUM_BUFFERS; i++)
		{
		//	alBufferData(buffers[i], AL_FORMAT_MONO16, sine, 44100*2, 44100);
		}

		if (alGetError() != AL_NO_ERROR)
		{
			printf_s("Couldn't buffer data\n");
			assert(false);
		}

		alSourceQueueBuffers(source, 1, buffers);
		alSourcePlay(source);

		if (alGetError() != AL_NO_ERROR)
		{
			printf_s("Couldn't start playback\n");
			assert(false);
		}
	}

	void Update(short* data, int size)
	{
		ALint state, processed;
		alGetSourcei(source, AL_SOURCE_STATE, &state);
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

		if (alGetError() != AL_NO_ERROR)
		{
			printf_s("Error checking source state\n");
			assert(false);
		}

		while (processed > 0)
		{
			ALuint buffer_id;

			alSourceUnqueueBuffers(source, 1, &buffer_id);
			processed--;

			alBufferData(buffer_id, AL_FORMAT_MONO16, data, size, 44100);
			alSourceQueueBuffers(source, 1, &buffer_id);

			if (alGetError() != AL_NO_ERROR)
			{
				printf_s("Error buffering data\n");
				assert(false);
			}
		}

		if (state != AL_PLAYING && state != AL_PAUSED)
		{
			ALint queued;
			alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

			if (queued)
				alSourcePlay(source);//buffer underrun, restart playback
		}
	}

	void Cleanup()
	{
		ALCcontext *context = alcGetCurrentContext();

		if (!context)
		{
			printf_s("Couldn't get current context for Cleanup()");
			return;
		}
		ALCdevice *device = alcGetContextsDevice(context);

		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		alcCloseDevice(device);
	}
};

short sine[44100];

void GenerateSineArray()
{
	double max_value = 32767.0;

	double increment = 440.0 / 44100.0;//A-440 at 44100hz

	double position = 0;

	for (int i = 0; i < 44100; i++)
	{
		sine[i] = max_value * sin(2.0 * 3.14159265359 * position);

		position += increment;
	}
}

#if WANT_THREADING

DWORD WINAPI AudioThread(LPVOID lpParam)
{
	OpenAlBackend backend;

	backend.Initialize();
	backend.CreateBuffersAndSources();
	GenerateSineArray();
	backend.Start();

	float max = 5;
	float x = -max, y = 1, z = 0;
	float direction = .01;

	for (;;){

		x += direction;

		if (x > max)
		{
			direction *= -1;
		}
		else if (x < -max)
		{
			direction *= -1;
		}

		alSource3f(backend.source, AL_POSITION, x, y, z);
		backend.Update(&sine[0], 44100);
		Sleep(1);
	}

	backend.Cleanup();

	return 1;
}

int main(int argc, char *argv[])
{
	DWORD thread_id;
	CreateThread(NULL, 0, AudioThread, NULL, 0, &thread_id);

	for (;;){
		Sleep(1);
	}
}

#else

int main(int argc, char *argv[]){

	OpenAlBackend backend;

	backend.Initialize();
	backend.CreateBuffersAndSources();
	GenerateSineArray();
	backend.Start();

	float max = 5;
	float x = -max, y = 1, z = 0;
	float direction = .01;

	for (;;){

		x += direction;

		if (x > max)
		{
			direction *= -1;
		}
		else if (x < -max)
		{
			direction *= -1;
		}

		alSource3f(backend.source, AL_POSITION, x, y, z);

		backend.Update(&sine[0], 44100);
		Sleep(1);
	}

	backend.Cleanup();
}

#endif