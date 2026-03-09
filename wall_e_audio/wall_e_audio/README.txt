WAV files from wall_e_audio/ — copied for LittleFS upload.

Code uses these files:
  - Wall-E charged.wav (boot)
  - Ooooh 3 - Wonder 1.wav (idle)
  - Deep Sigh 1.wav (idle alt)
  - Ahhh 1 - Understanding.wav (curious)
  - High Warning Tones.wav (estop)

IMPORTANT: Convert to 48000 Hz, 16-bit mono for correct playback:
  ffmpeg -i "input.wav" -ar 48000 -ac 1 "output.wav"

Then: Tools > Upload LittleFS to ESP32
