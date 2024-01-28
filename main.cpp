#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "libs/glfw/include/GLFW/glfw3.h"
#include "implot.h"
#include "implot_internal.h"
#include "wave.h"
#include <fstream>


//Window object
GLFWwindow* window;
float displayX = 0.0f;
float displayY = 0.0f;

//Data needed for plot
std::vector<float> amplitude_vector_channel1;
std::vector<float> amplitude_vector_channel2;
std::vector<float> audio_time;

//Window and ImGui setup code
void setup()
{
    // Initialize GLFW and configure
    glfwInit();
    //OpenGL Version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //Use CORE profile of OpenGL (removes deprecated features)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

    //Get monitor information
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    displayX = mode->width / 2;
    displayY = mode->height / 2;

    // Create window
    window = glfwCreateWindow((displayX) + (displayX * 2 * 0.10), (displayY), "Assignment 1", nullptr, nullptr);
    //Verify window was created
    if (window == nullptr)
        return;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    //Enable cursor inside window
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

//Window and ImGui cleanup code
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

//Helper function used by readFile() to iterate until "data" is found from startPos onwards.
int findData(std::ifstream& inputFile, int startPos)
{
    bool dataFound = false;
    int subchunk2SizePosition = 0;
    char dataPattern[] = "data";

    //File must be open
    if (!inputFile.is_open())
        return -1;

    //Get the File Size
    inputFile.seekg(-4, std::ios::end);
    int fileSize = inputFile.tellg();

    //Place cursor to desired location
    inputFile.seekg(startPos, std::ios::beg);

    //Seek through file to find "data". Some files have multiple "data"s. Find the one that is not followed by all zeros (some of this logic is in readFile)
    for(int i = startPos; i < fileSize; i++)
    {
        //Read 4 bytes, store values in char array
        char buffer[4];

        //Read 4 bytes, this moves cursor 4 bytes forwards after reading
        inputFile.read(buffer, sizeof(buffer)); 

        //Check to see if buffer == "data"
        if (std::memcmp(buffer, dataPattern, 4) == 0)
        {
            dataFound = true;
            break;
        }

        //If data was not found, move back 3 bytes to check the next 4 byte set
        else
            inputFile.seekg(-3, std::ios::cur);
    }

    //If we reached the end of the file without finding a valid data chunk then stop the program
    if (!dataFound)
    {
        std::cout << "ERROR: Read " << inputFile.tellg() << "/" << fileSize << " Bytes. Valid data chunk cannot be found!" << std::endl;
        return -1;
    }

    //Save the cursor position for the start of audio data size (the chunk followed by "data")
    subchunk2SizePosition = inputFile.tellg();
    return subchunk2SizePosition;
}

//Read the file by bytes to extract data from the .wav file
int readFile(std::string fileName, Wave& wave, bool performance)
{
    //Open Wav file and read
    amplitude_vector_channel1.clear();
    amplitude_vector_channel2.clear();
    audio_time.clear();

    std::ifstream inputFile(fileName, std::ifstream::binary);
    if (inputFile.is_open()) 
    {
        //GET HEADER INFO
        char header[36];

        //Read first 36 bytes
        inputFile.seekg(0, std::ios::beg);
        inputFile.read(header, sizeof(header));
        
        //Gather "fmt" sub-chunk info
        wave.subchunk1_size = *reinterpret_cast<int*>(&header[16]);
        wave.audio_format = *reinterpret_cast<short*>(&header[20]);
        wave.num_channels = *reinterpret_cast<short*>(&header[22]);
        wave.sample_rate = *reinterpret_cast<int*>(&header[24]);
        wave.byte_rate = *reinterpret_cast<int*>(&header[28]);
        wave.block_align = *reinterpret_cast<short*>(&header[32]);
        wave.bits_per_sample = *reinterpret_cast<short*>(&header[34]);

        //The size of each sample in bytes if wave.sample_size = 4, sample size is 4, channel 1 size = 2, channel 2 size = 2
        wave.sample_size = (wave.bits_per_sample / 8) * wave.num_channels;

        //Create a buffer to read the data size chunk
        char dataSizeBuffer[4];
        //Read the data chunk following "data"
        //If that integer is 0, the file is messed up and probably has a data chunk elsewhere as seen in audio2.wav
        int subchunk2SizePosition = 0;
        
        //Specific to assignment, audio2.wav has an issue where it has 2 "data" chunks, one is all zeros, the other has actual data.
        //By reading the size, we can check if the size of subchunk2_size is 0, if it is no data exists. Another "data" chunk may exist
        //so continue iterating until it is found. If its not found and all bytes are read then return an error
        while (wave.subchunk2_size == 0)
        {
            //Find the position of the "data" chunk
            subchunk2SizePosition = findData(inputFile, subchunk2SizePosition);
            if (subchunk2SizePosition == -1)
            {
                inputFile.close();
                std::cout << "ERROR: " << fileName << " cannot be read." << std::endl;
                return -1;
            }
            //Read the next 4 bytes (the size chunk)
            inputFile.read(dataSizeBuffer, 4);
            //Cast the value into an integer
            wave.subchunk2_size = *reinterpret_cast<int*>(&dataSizeBuffer);
        }
        
        wave.number_of_samples = wave.subchunk2_size / (wave.num_channels * (wave.bits_per_sample / 8));
        wave.duration = (float)wave.number_of_samples / (float)wave.sample_rate;

 
        int sampleCounter = 1;
        char* bytes = new char[wave.block_align];
        inputFile.seekg(subchunk2SizePosition, std::ios::beg);

        //Every 2 samples is 4 bytes. Read every n bytes where n is a multiple of 4 (skip some bytes for the sake of performance due to ImPlot)
        int n = 1;
        if (performance == true)
        {
            n = wave.block_align * 2;
            std::cout << "Performance Mode on, reading every " << n << " samples" << std::endl;
        }

        //This allows support for 4, 6, and 8 byte block aligns (ie 
        short amplitude1s;
        short amplitude2s;
        int amplitude1i;
        int amplitude2i;
        float amplitude1f;
        float amplitude2f;

        int halfway_byte = wave.block_align / 2;
        while (inputFile.read(bytes, wave.block_align)) 
        {
            
            //Read only every nth set of 4 bytes (Read every other nth sample)
            if (sampleCounter % n == 0) 
            {

                //Set sample values
                switch (wave.block_align)
                {
                    //8-bit depth (not yet implemented)

                    //16-bit depth
                    case 4:
                        amplitude1s = *reinterpret_cast<short*>(&bytes[0]);
                        amplitude2s = *reinterpret_cast<short*>(&bytes[halfway_byte]);
                        amplitude_vector_channel1.push_back(static_cast<float>(amplitude1s));
                        amplitude_vector_channel2.push_back(static_cast<float>(amplitude2s));
                        break;
                    //32-bit depth
                    case 6:
                        //Special case: no data type of 3 bytes exist. Shifting method used instead
                        //Little Endian (https://stackoverflow.com/questions/9896589/how-do-you-read-in-a-3-byte-size-value-as-an-integer-in-c)
                        amplitude1i = bytes[2] + (bytes[1] << 8) + (bytes[0] << 16);
                        amplitude2i = bytes[5] + (bytes[4] << 8) + (bytes[3] << 16);
                        amplitude_vector_channel1.push_back(static_cast<float>(amplitude1i));
                        amplitude_vector_channel2.push_back(static_cast<float>(amplitude2i));
                        break;
                    //32-bit depth
                    case 8:
                        amplitude1f = *reinterpret_cast<float*>(&bytes[0]);
                        amplitude2f = *reinterpret_cast<float*>(&bytes[halfway_byte]);
                        amplitude_vector_channel1.push_back(static_cast<float>(amplitude1f));
                        amplitude_vector_channel2.push_back(static_cast<float>(amplitude2f));
                        break;

                }

                audio_time.push_back( sampleCounter );
            }
            sampleCounter++;
        }

        //Less elegant, but a more consistent way to measure time
        wave.number_of_samples = sampleCounter;
        wave.duration = (float)wave.number_of_samples / (float)wave.sample_rate;

    }
    else {
        std::cerr << "Error: Unable to open the file: " << fileName << std::endl;
        return -1;
    }
    inputFile.close();
    std::cout << "Loaded Succesfully" << std::endl;
    return 0;
}

void helpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

//Main code
int main()
{ 
    //Setup Graphical User Interface
    setup();

    bool isFileOpen = false;
    static char file_name_buffer[256] = "test samples/Q1/";
    std::string file_name = "";
    bool performance = false;
    Wave wave;

    // Main loop        

    while (!glfwWindowShouldClose(window))
    {
        //Reset viewport
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //Wave Form Window
        if (isFileOpen)
        {
            //Set waveform window size and position
            ImGui::SetNextWindowSize(ImVec2(displayX, displayY), ImGuiCond_Once);
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);

            //Display waveform
            ImGui::Begin(("Wave Form of: "+ file_name).c_str() );
            {

                //Channel 1 Plot
                if (ImPlot::BeginPlot("Channel 1")) {
                    ImPlot::SetupAxes("Samples", "Amplitude");
                    ImPlot::PlotLine("", audio_time.data(), amplitude_vector_channel1.data(), audio_time.size());
                    ImPlot::EndPlot();
                }

                //Channel 2 Plot
                if (ImPlot::BeginPlot("Channel 2")) {
                    ImPlot::SetupAxes("Samples", "Amplitude");
                    ImPlot::PlotLine("", audio_time.data(), amplitude_vector_channel2.data(), audio_time.size());
                    ImPlot::EndPlot();
                }

            }
            ImGui::End();

            //Set partner window size and position
            ImGui::SetNextWindowSize(ImVec2((displayX * 2  * 0.10), displayY), ImGuiCond_Once);
            ImGui::SetNextWindowPos(ImVec2(displayX, 0), ImGuiCond_Once);

            //Display file properties in a partner window
            ImGui::Begin("Properties");
            {
                ImGui::Text("Subchunk1 Size:\n%i", wave.subchunk1_size);
                ImGui::Text("Audio Format:\n%i", wave.audio_format);
                ImGui::Text("Number of Channels:\n%i", wave.num_channels);
                ImGui::Text("Sample Rate (kHz):\n%i", wave.sample_rate);
                ImGui::Text("Byte Rate:\n%i", wave.byte_rate);
                ImGui::Text("Bytes Per Sample:\n%i", wave.block_align);
                ImGui::Text("Bits Per Sample:\n%i", wave.bits_per_sample);
                ImGui::Text("Number of Samples:\n%i", wave.number_of_samples);
                ImGui::Text("Duration (s):\n%f", wave.duration);
                ImGui::Spacing();
                ImGui::Text("\nINFO:\n");
                ImGui::Text("\nClick on wave form and drag to move.\n\nUse scroll wheel to adjust zoom.");
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Text("Return To File Select");
                if (ImGui::Button("Return"))
                    isFileOpen = false;


            }
            ImGui::End();

        }
        //Input File Window
        else
        {
            //Set input file window size and position
            ImGui::SetNextWindowSize(ImVec2(displayX / 2, displayY/4), ImGuiCond_Once);
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));

            //Display input file window
            ImGui::Begin("InputWindow");
            {
                ImGui::Text("File Location");
                if (ImGui::InputText("##File Location: ", file_name_buffer, 256))
                    file_name = file_name_buffer;
                ImGui::SameLine(); helpMarker(
                    "Path relative to exe or solution directory.\n");
                ImGui::Spacing();

                if (ImGui::Button("Submit")) {
                    if (readFile(file_name_buffer, wave, performance) == 0) {
                        isFileOpen = true;
                    }
                }

                ImGui::SameLine();
                /*
                ImGui::Checkbox("Performance Mode", &performance);
             
                ImGui::SameLine(); helpMarker(
                    "Performance mode reads every 16th sample. This useful for displaying large"
                    " files with many samples on weaker hardware."
                    " Perceptable visual loss is minimal for large files when zoomed out."
                    " Leaving this unchecked, the program will read and display all samples."
                );
                ImGui::Spacing();
                */

                //Some shortcuts for easier testing
                ImGui::Spacing();
                ImGui::Text("Shortcuts");
                if (ImGui::Button("audio1.wav")) {
                    if (readFile("test samples/Q1/audio1.wav", wave, performance) == 0) {
                        file_name = "test samples/Q1/audio1.wav";
                        isFileOpen = true;
                    }
                }

                if (ImGui::Button("audio2.wav"))
                {
                    if (readFile("test samples/Q1/audio2.wav", wave, performance) == 0) {
                        file_name = "test samples/Q1/audio2.wav";
                        isFileOpen = true;
                    }
                }
            }     
            ImGui::End();
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