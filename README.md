Devloped in Visual Studio 2022 using C++ 14 Standard.
# About

This program displays the waveform of .wav files by parsing through .wav files and gathering the audio data. It displays this data as a waveform using Dear ImGui alongside ImPlot with GLFW and OpenGL serving as the renderer. The plot itself is drawn by the PlotLine function built into ImPlot. This function accepts two vectors, an x axis vector and a right axis vector.

The program also presents the user with some key .wav metadata info including the duration of the audio, the sample rate (sampling frequency) and the total number of samples.

# How To Run
Ensure you run this program on a 64 bit Windows computer with updated Microsoft Visual C++ Redistributables x64 2015, 2017, 2019, and 2022. <br /><br />https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#visual-studio-2015-2017-2019-and-2022<br /><br />This program also uses OpenGL 3, almost all modern computers come with OpenGl already installed in the graphics processor, so no action regarding this is expected from the user. <br /><br />All .wav files must be located in the same directory as the .exe. They can be in a folder as long as that folder is in the same folder as the .exe or the solution, depending on the method you choose to run.




The program contains shortcuts for easier use but only for the test samples provided on Canvas.
These shortcuts must be stored in "test samples/Q1" where "test samples" is in the same directory as the .exe and the solution.

### Please note that the .exe version does have better performance so I recommend using that vs running the program in Visual Studio

# Screenshots
![image](https://github.com/rsolis096/CMPT-365-Assignment-1-Q1/assets/63280140/4cc81427-b809-4999-b921-0d3b06e1f66c)

![audio1](https://github.com/rsolis096/CMPT-365-Assignment-1-Q1/assets/63280140/b59f6391-1f63-4c79-b70f-f025d4d3e4ab)

![audio2](https://github.com/rsolis096/CMPT-365-Assignment-1-Q1/assets/63280140/97038da9-f2ab-4fab-b5a6-b6d033071c41)
