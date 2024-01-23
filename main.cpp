#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "libs/glfw/include/GLFW/glfw3.h"
#include "implot.h"
#include "implot_internal.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>


GLFWwindow* window;


//Source for this information from http://soundfile.sapp.org/doc/WaveFormat/

//Riff Chunk
const std::string chunk_id = "RIFF";
const std::string chunk_size = "----";
const std::string format = "WAVE";

//FMT sub-chunk
const std::string subchunk1_id = "fmt ";
const int subchunk1_size = 16;
const short audio_format = 1;
const short num_channels = 2;
const int sample_rate = 44100;
const int byte_rate = sample_rate * num_channels * (subchunk1_size / 8);
const short block_align = num_channels * (subchunk1_size / 8);
const short bits_per_sample = 16;

//Data sub-chunk
const std::string subchunk2_id = "data";
const std::string subchunk2_size = "----";
const int duration = 2;
const int max_amplitude = 32760;
const double frequency = 250;

//Data needed for plot
std::vector<float> amplitude_vector_channel1;
std::vector<float> amplitude_vector_channel2;
std::vector<float> audio_time;

void setup()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //OpenGL Version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Use CORE profile of OpenGL (Modern profile with deprecated features removed)
    // Set desired depth buffer size
    glfwWindowHint(GLFW_DEPTH_BITS, 32);

    // Create window with graphics context
    window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

void cleanup()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

//file: file we are writing to
//value" value we are writing to file
//byte_size: size of byte
//template <typename T> //short or int
void write_as_bytes(std::ofstream &file, int value, int byte_size)
{
    // reinterpret_cast<const char*> casts &value into a pointer to a const char, this is required by the write method in ofstream
    for (int i = 0; i < byte_size; i++)
    {
        //We have byte_size number of bytes.
        //We shift the bit pattern by 8 * i, this isolates the relavent byte we need by shifting it towards LSB
        //Perform bitwise & with 1111 1111, this gives us the byte we need 
        char byte = static_cast<char>((value >> (8 * i)) & 0xFF);
        //std::cout << std::hex << (static_cast<int>(byte) & 0xFF) << " "; //Use this to visualize data being written
        file.write(&byte, 1);
    }
}

void createFile()
{
    //Create Wav file
    std::ofstream wav;
    wav.open("test.wav", std::ios::binary);
    //Write to wav file
    if (wav.is_open()) {

        //Riff Chunk
        wav << chunk_id;
        wav << chunk_size;
        wav << format;

        //FMT sub-chunk
        wav << subchunk1_id;
        write_as_bytes(wav, subchunk1_size, 4);
        write_as_bytes(wav, audio_format, 2);
        write_as_bytes(wav, num_channels, 2);
        write_as_bytes(wav, sample_rate, 4);
        write_as_bytes(wav, byte_rate, 4);
        write_as_bytes(wav, block_align, 2);
        write_as_bytes(wav, bits_per_sample, 2);

        //Data sub-chunk
        wav << subchunk2_id;
        wav << subchunk2_size;

        //Save location of previous write
        int start_audio = wav.tellp();

        //Generate audio data
        int number_of_samples = sample_rate * duration;
        for (int i = 0; i < number_of_samples; i++)
        {
            double amplitude = (double)i / sample_rate * max_amplitude;
            double value = sin((2 * 3.14 * i * frequency) / sample_rate);

            double channel1 = amplitude * value / 2;
            double channel2 = (max_amplitude - amplitude) * value;

            //Data needed to plot
            audio_time.push_back((float)i /2);
            amplitude_vector_channel1.push_back((float)channel1);
            amplitude_vector_channel2.push_back((float)channel2);

            write_as_bytes(wav, channel1, 2);
            write_as_bytes(wav, channel2, 2);
        }
        int end_audio = wav.tellp();

        //write to subchunk2_size
        wav.seekp(start_audio - 4);
        write_as_bytes(wav, end_audio - start_audio, 4);

        //write to chunk_size
        wav.seekp(4, std::ios::beg);
        write_as_bytes(wav, end_audio - 8, 4);
    }
    wav.close();
}

//Helper function used to iterate until "data" is found from startPos onwards.
int findData(std::ifstream& inputFile, int startPos)
{
    bool dataFound = false;
    int subchunk2SizePosition = 0;
    char dataPattern[] = "data";

    //File must be open
    if (!inputFile.is_open())
        return 0;

    //Place cursor to desired location
    inputFile.seekg(startPos, std::ios::beg);

    //Seek through file to find "data". Some files have multiple "data"s. Find the one that is not followed by all zeros (some of this logic is in readFile)
    while (!dataFound)
    {
        //Read 4 bytes, store values in char array
        char buffer[4];
        inputFile.read(buffer, sizeof(buffer));

        //Check to see if buffer == "data"
        if (std::memcmp(buffer, dataPattern, 4) == 0)
            dataFound = true;

        //If data was not found, move back 3 bytes to check the next 4 byte set (important for overlapping memory addresses)
        else
            inputFile.seekg(-3, std::ios::cur);
    }
    //Save the cursor position for the start of audio data size (the chunk followed by "data")
    subchunk2SizePosition = inputFile.tellg();
    std::cout << "Data starts at is at: " << subchunk2SizePosition << std::endl;
    return subchunk2SizePosition;
}

void readFile() 
{
    //Open Wav file and read
    amplitude_vector_channel1.clear();
    amplitude_vector_channel2.clear();
    audio_time.clear();

    std::ifstream inputFile("test samples/Q1/music.wav", std::ifstream::binary);
    if (inputFile.is_open()) 
    {
        //GET HEADER INFO
        char header[36];
        inputFile.seekg(0, std::ios::beg);
        inputFile.read(header, sizeof(header));
        
        //"fmt" sub-chunk
        int inputSubChunk1Size = *reinterpret_cast<int*>(&header[16]);
        int inputAudioFormat = *reinterpret_cast<short*>(&header[20]);  
        int inputNumChannels = *reinterpret_cast<short*>(&header[22]);
        int inputSampleRate = *reinterpret_cast<int*>(&header[24]);
        int inputByteRate = *reinterpret_cast<int*>(&header[28]);
        int inputBlockAlign = *reinterpret_cast<short*>(&header[32]);
        int inputBitsPerSample = *reinterpret_cast<short*>(&header[34]);

        std::cout << "Subchunk1 Size: \t\t" << inputSubChunk1Size << std::endl;
        std::cout << "Audio Format: \t\t\t" << inputAudioFormat << std::endl;
        std::cout << "Num Channels: \t\t\t" << inputNumChannels << std::endl;
        std::cout << "Sample Rate: \t\t\t" << inputSampleRate << std::endl;
        std::cout << "Byte Rate: \t\t\t" << inputByteRate << std::endl;
        std::cout << "Block Align: \t\t\t" << inputBlockAlign << std::endl;
        std::cout << "Bits Per Sample: \t\t" << inputBitsPerSample << std::endl;
        std::cout << std::endl;

        //Move to the data chunk and read it
        int subchunk2SizePosition = findData(inputFile,36);
        char dataSizeBuffer[4];
        inputFile.seekg(subchunk2SizePosition, std::ios::beg);
        inputFile.read(dataSizeBuffer, sizeof(dataSizeBuffer));
        int inputSubChunk2_Size = *reinterpret_cast<int*>(&dataSizeBuffer[0]);
        while (inputSubChunk2_Size == 0)
        {
            subchunk2SizePosition = findData(inputFile, subchunk2SizePosition);
            inputFile.seekg(subchunk2SizePosition, std::ios::beg);
            inputFile.read(dataSizeBuffer, sizeof(dataSizeBuffer));
            inputSubChunk2_Size = *reinterpret_cast<int*>(&dataSizeBuffer[0]);
        }
        std::cout << "Subchunk2 Size (Bytes): " << inputSubChunk2_Size << std::endl;
        

  
        int inputNumSamples = inputSubChunk2_Size / (inputNumChannels * (inputBitsPerSample / 8));


        // Initialize audioData size to hold the correct number of "bytes"
        std::vector<short> audioData(inputSubChunk2_Size);

        inputFile.seekg(subchunk2SizePosition, std::ios::beg);
        //Read data of audio data chunk, read all inputSubChunk2_Size bytes, store them in audioData vector
        inputFile.read(
            reinterpret_cast<char*>(audioData.data()), 
            inputSubChunk2_Size
        );

        //Cast the data into a float for ImGui plot compatability
        for (int i = 0; i < audioData.size(); i++)
        {
            //Each element of audioData refers to a byte
            if(i%2 == 0)
                amplitude_vector_channel1.push_back((float)(audioData[i]));
            else
                amplitude_vector_channel2.push_back((float)(audioData[i]));
        }

        for (int i = 1; i < inputNumSamples; i++)
            audio_time.push_back(i);






    }
    else {
        std::cerr << "Error: Unable to open the file." << std::endl;
        return;
    }
    inputFile.close();
}

//Main code
int main()
{
    //createFile();
    readFile();
    
    //Graphical User Interface
    setup();
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f); //sets the clear color for the color buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //Channel 1 Plot
        if (ImPlot::BeginPlot("Channel 1")) {
            ImPlot::SetupAxes("x", "y");
            ImPlot::PlotLine("f(x)", audio_time.data(), amplitude_vector_channel1.data(), audio_time.size());
            ImPlot::EndPlot();
        }

        //Channel 2 Plot
        if (ImPlot::BeginPlot("Channel 2")) {
            ImPlot::SetupAxes("x", "y");
            ImPlot::PlotLine("f(x)", audio_time.data(), amplitude_vector_channel2.data(), audio_time.size());
            ImPlot::EndPlot();
        }

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    //Cleanup and close program
    cleanup();
    
    return 0;
}