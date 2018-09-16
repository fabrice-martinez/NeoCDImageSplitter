# NeoCD Image Splitter

## What is it ?

This application is a tool to split a .BIN / .CUE type CD-ROM image and convert it to .ISO / .WAV / .CUE files. To save space you can then compress the .WAV files to .FLAC and the image will still work with **NeoCD**.

## How to use

Start the application, click **Load CUE File** load the .CUE file of the image you want to convert, then click **Create Split File Version**. The program will ask you to select a folder to save the files to and the base filename. Files created will be named like this: `[Track Number]-[Base Name].[Extension]`. You can then use a program like **Foobar2000** to compress the .WAV files to .FLAC then edit the .CUE file to change all .WAV file extensions to .FLAC.

## Build

This program is made using Qt5, use **qmake** to generate the makefile then build it with **make**. 
