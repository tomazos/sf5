#include <SDL2/SDL.h>
#include <sys/types.h>
#include <cmath>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

#include "dvc/log.h"
#include "spk/program.h"

class AudioWave {
 public:
  virtual double sample(double t, size_t channel) = 0;
};
using PAudioWave = std::unique_ptr<AudioWave>;

class RoundWave : public AudioWave {
 public:
  double sample(double t, size_t channel) override {
    return std::sin(t * 2 * M_PI);
  }
};

PAudioWave round_wave() { return std::make_unique<RoundWave>(); }

class SquareWave : public AudioWave {
 public:
  double sample(double t, size_t channel) override {
    return std::fmod(t, 1.0) < 0.5 ? 1.0 : -1.0;
  };
};

PAudioWave square_wave() { return std::make_unique<SquareWave>(); }

class TriangleWave : public AudioWave {
  double sample(double t, size_t channel) override {
    double d = std::fmod(t, 1.0);
    if (d < 0.25)
      return 4 * d;
    else if (d < 0.75)
      return 2 + (-4 * d);
    else
      return 4 * d - 2;
  }
};

PAudioWave triangle_wave() { return std::make_unique<TriangleWave>(); }

class SawWave : public AudioWave {
  double sample(double t, size_t channel) override {
    double d = std::fmod(t, 1.0);
    return 2 * d - 1;
  };
};

PAudioWave saw_wave() { return std::make_unique<SawWave>(); }

class ReshapeWave : public AudioWave {
 public:
  ReshapeWave(PAudioWave subwave, double amplitude, double frequency)
      : subwave(std::move(subwave)),
        amplitude(amplitude),
        frequency(frequency) {}

  double sample(double t, size_t channel) override {
    return amplitude * subwave->sample(t * frequency, channel);
  }

 private:
  PAudioWave subwave;
  double amplitude;
  double frequency;
};

PAudioWave reshape(PAudioWave subwave, double amplitude, double frequency) {
  return std::make_unique<ReshapeWave>(std::move(subwave), amplitude,
                                       frequency);
}

class SilentWave : public AudioWave {
 public:
  double sample(double t, size_t channel) override { return 0; }
};

PAudioWave silence() { return std::make_unique<SilentWave>(); }

class AddWave : public AudioWave {
 public:
  AddWave(PAudioWave a, PAudioWave b) : a(std::move(a)), b(std::move(b)) {}

  double sample(double t, size_t channel) override {
    return a->sample(t, channel) + b->sample(t, channel);
  }

 private:
  PAudioWave a, b;
};

PAudioWave operator+(PAudioWave a, PAudioWave b) {
  return std::make_unique<AddWave>(std::move(a), std::move(b));
}

class AudioSystem {
 public:
  AudioSystem() {
    SDL_AudioSpec desired, obtained;
    SDL_memset(&desired, 0, sizeof(desired));
    desired.format = AUDIO_F32;
    desired.freq = 48000;
    desired.channels = 2;
    desired.samples = 128;
    desired.callback = generate_audio;
    desired.userdata = this;

    device_id = SDL_OpenAudioDevice(nullptr /*device*/, false /*iscapture*/,
                                    &desired, &obtained, 0 /*allow changes*/);
    SDL_PauseAudioDevice(device_id, 0 /*pause_on*/);
    DVC_ASSERT(device_id != 0);
  }

  void set_wave(PAudioWave wave) {
    std::lock_guard lock(mu);
    this->wave = std::move(wave);
  }
  ~AudioSystem() { SDL_CloseAudioDevice(device_id); }

 private:
  static void generate_audio(void* userdata, Uint8* char_stream, int len) {
    ((AudioSystem*)userdata)->generate_audio(char_stream, len);
  }

  void generate_audio(Uint8* char_stream, int len) {
    float* float_stream = (float*)char_stream;

    static size_t current_sample = 0;
    DVC_ASSERT(len % (4 * 2) == 0);
    size_t num_samples = len / (4 * 2);

    for (size_t i = 0; i < num_samples; i++, current_sample++) {
      for (size_t channel = 0; channel < 2; channel++) {
        float_stream[i * 2 + channel] =
            generate_sample(double(current_sample) / 48000, channel);
      }
    }
  }

  double generate_sample(double t, size_t channel) {
    std::lock_guard lock(mu);
    if (wave)
      return wave->sample(t, channel);
    else
      return 0;
  }

  std::mutex mu;
  PAudioWave wave;

  SDL_AudioDeviceID device_id;
};

class smoke_audio : public spkx::program {
 public:
  using spkx::program::program;

  AudioSystem sys;

  std::array<bool, 4> playing;
  std::array<double, 4> freqs = {261.626, 293.665, 329.628, 349.228};

  void set_wave() {
    PAudioWave w = silence();
    for (int i = 0; i < 4; i++)
      if (playing[i]) w = std::move(w) + reshape(round_wave(), 0.25, freqs[i]);
    sys.set_wave(std::move(w));
  }

  void start_playing(int note) {
    DVC_LOG("start_playing ", note);

    set_wave();
  }

  void stop_playing(int note) {
    DVC_LOG("stop playing ", note);

    set_wave();
  }

  void key_change(const keyboard_event& event, bool down) {
    static std::map<SDL_Keycode, int> keycode_to_n = {
        {SDLK_a, 0}, {SDLK_s, 1}, {SDLK_d, 2}, {SDLK_f, 3}};

    auto it = keycode_to_n.find(event.keysym.sym);

    if (it != keycode_to_n.end()) {
      int note = it->second;

      if (playing[note] != down) {
        playing[note] = down;
        if (down)
          start_playing(note);
        else
          stop_playing(note);
      }
    }
  }

  void key_down(const keyboard_event& event) override {
    if (event.keysym.sym == SDLK_q) {
      shutdown();
      return;
    }
    key_change(event, true);
  }

  void key_up(const keyboard_event& event) override {
    key_change(event, false);
  }

 private:
};

int main(int argc, char** argv) {
  smoke_audio program(argc, argv);

  program.run();
}
