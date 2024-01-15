#!/bin/bash

#https://github.com/F5OEO/rpitx
#https://github.com/F3lda/rpitx-FM-AM

#make ../pifmrds
#sudo ../pifmrds -freq 102.0 -audio - < <(while true; do cat ../src/pifmrds/stereo_44100.wav; done)
#sudo ../pifmrds -raw -freq 102.0 -audio - < <(while true; do cat ../src/pifmrds/stereo_44100.wav; done)

### Only one transmission at time!

echo
echo "Make rpitx and cd rpitx"
echo
echo "Commands:"
echo "cat src/pifmrds/stereo_44100.wav | sudo ./pifmrds -freq 102.0 -audio - "
echo "sudo ./pifmrds -freq 102.0 -audio - < <(cat src/pifmrds/stereo_44100.wav)"
echo
echo "sudo ./pifmrds -freq 102.0 -audio - < <(while true; do cat src/pifmrds/stereo_44100.wav; done)"
echo "sudo ./pifmrds -raw -freq 102.0 -audio - < <(while true; do cat src/pifmrds/stereo_44100.wav; done)"
echo
echo
echo "FFMPEG:"
echo "ffmpeg -i '../MUSIC/Modern Talking - Cheri Cheri Lady (T-Beat Rework) 2k23.mp3' -f wav - | sudo ./pifmrds -freq 102.0 -audio -"
echo "sudo ./pifmrds -freq 102.0 -audio - < <(ffmpeg -i '../MUSIC/Modern Talking - Cheri Cheri Lady (T-Beat Rework) 2k23.mp3' -f wav -)"
echo
echo "Infinite LOOP:"
echo "ffmpeg -stream_loop -1 -i 'src/pifmrds/stereo_44100.wav' -f wav - | sudo ./pifmrds -freq 102.0 -audio -"
echo "sudo ./pifmrds -freq 102.0 -audio - < <(ffmpeg -stream_loop -1 -i 'src/pifmrds/stereo_44100.wav' -f wav -)"
echo
echo "ffmpeg -stream_loop -1 -i '../MUSIC/Modern Talking - Cheri Cheri Lady (T-Beat Rework) 2k23.mp3' -f wav - | sudo ./pifmrds -freq 102.0 -audio -"
echo "sudo ./pifmrds -freq 102.0 -audio - < <(ffmpeg -stream_loop -1 -i '../MUSIC/Modern Talking - Cheri Cheri Lady (T-Beat Rework) 2k23.mp3' -f wav -)"
echo
echo
echo
echo "AM 10 MHz"
echo "./testnfm.sh '10e3'"
echo "(while true; do cat sampleaudio.wav; done) | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo ./rpitx -i- -m RF -f 10e3"
echo
echo "ffmpeg -stream_loop -1 -i '../MUSIC/Creedence Clearwater Revival - Fortunate Son (Official Music Video).mp3' -ac 1 -ar 48000 -acodec pcm_s16le -f wav - | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo ./rpitx -i- -m RF -f 10e3"
echo
echo
echo "AM 1600 KHz"
echo "ffmpeg -stream_loop -1 -i '../MUSIC/Creedence Clearwater Revival - Fortunate Son (Official Music Video).mp3' -ac 1 -ar 48000 -acodec pcm_s16le -f wav - | csdr convert_i16_f | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 | sudo ./rpitx -i- -m RF -f 1600"
echo


#https://video.stackexchange.com/questions/18377/infinite-video-loop-in-ffmpeg
#https://superuser.com/questions/826669/ffmpeg-get-mono-wav-audio-8khz-16-bit-out-of-mp4-video
#https://ffmpeg-user.ffmpeg.narkive.com/EeOrSpJr/please-can-someone-tell-me-why-this-ffmpeg-command-stopped-working
#https://stackoverflow.com/questions/39461643/avcodec-open2-pcm-channels-out-of-bounds
#https://unix.stackexchange.com/questions/176886/convert-to-wav-using-ffmpeg-for-pipe-into-lame
#https://lindevs.com/install-ffmpeg-on-raspberry-pi


echo
echo "BLUETOOTH"
echo "ffmpeg -use_wallclock_as_timestamps 1 -f pulse -i default -ac 2 -f wav - | sudo ./pifmrds -ps "HistoRPi" -rt "HistoRPi: live FM-RDS transmission from the RaspberryPi" -freq 102.0 -audio -"
echo "sudo ./pifmrds -ps 'HistoRPi' -rt 'HistoRPi: live FM-RDS transmission from the RaspberryPi' -freq 102.0 -audio - < <(ffmpeg -use_wallclock_as_timestamps 1 -f pulse -i default -ac 2 -f wav -)"
echo

#https://trac.ffmpeg.org/wiki/Capture/PulseAudio
#pactl list short sources
#https://superuser.com/questions/1326829/non-monotonous-dts-in-output-stream
