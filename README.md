# SimpleVMS

A simple Video Management Software (VMS) bases on ffmpeg

I modified ffmpeg and added custom protocol to handle file I/O in memory. So i can generate fragment mp4 files that were compatible with HTML5 MSE. CPU usage is very low because video stream is copied from camera, ffmpeg only does demuxing and muxing.

This project currently run on Microsoft Windows
