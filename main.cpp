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

    //Get monitor information
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    displayX = mode->width / 2;
    displayY = mode->height / 2;

    // Create window with graphics context to ensure fit on other screens
    window = glfwCreateWindow((displayX) + (displayX * 2 * 0.10), (displayY), "Assignment 1", nullptr, nullptr);
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

    //Window Dimensions
    std::cout << mode->width <<", " << mode->height <<  std::endl;
    std::cout << displayX << ", " << displayY << std::endl;

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
    //Initialize wave object.
    Wave wave;
    //Create Wav file
    std::ofstream outputFile;
    outputFile.open("test.wav", std::ios::binary);
    //Write to wav file
    if (outputFile.is_open()) {

        //Riff Chunk
        outputFile << wave.chunk_id;
        outputFile << wave.chunk_size;
        outputFile << wave.format;

        //FMT sub-chunk
        outputFile << wave.subchunk1_id;
        write_as_bytes(outputFile, wave.subchunk1_size, 4);
        write_as_bytes(outputFile, wave.audio_format, 2);
        write_as_bytes(outputFile, wave.num_channels, 2);
        write_as_bytes(outputFile, wave.sample_rate, 4);
        write_as_bytes(outputFile, wave.byte_rate, 4);
        write_as_bytes(outputFile, wave.block_align, 2);
        write_as_bytes(outputFile, wave.bits_per_sample, 2);

        //Data sub-chunk
        outputFile << wave.subchunk2_id;
        outputFile << wave.subchunk2_size;

        //Save location of previous write
        int start_audio = outputFile.tellp();

        //Generate audio data
        int number_of_samples = wave.sample_rate * wave.duration;
        for (int i = 0; i < number_of_samples; i++)
        {
            double amplitude = (double)i / wave.sample_rate * wave.max_amplitude;
            double value = sin((2 * 3.14 * i * wave.frequency) / wave.sample_rate);

            double channel1 = amplitude * value / 2;
            double channel2 = (wave.max_amplitude - amplitude) * value;

            //Data needed to plot
            audio_time.push_back((float)i /2);
            amplitude_vector_channel1.push_back((float)channel1);
            amplitude_vector_channel2.push_back((float)channel2);

            write_as_bytes(outputFile, channel1, 2);
            write_as_bytes(outputFile, channel2, 2);
        }
        int end_audio = outputFile.tellp();

        //write to subchunk2_size
        outputFile.seekp(start_audio - 4);
        write_as_bytes(outputFile, end_audio - start_audio, 4);

        //write to chunk_size
        outputFile.seekp(4, std::ios::beg);
        write_as_bytes(outputFile, end_audio - 8, 4);
    }
    outputFile.close();
}

//Helper function used to iterate until "data" is found from startPos onwards.
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
        std::cout << "ERROR: Read " << inputFile.tellg() << "/" << fileSize << " Bytes" << std::endl;
        std::cout << "Valid data chunk cannot be found. Check your file and restart the program." << std::endl;
        abort();
    }

    //Save the cursor position for the start of audio data size (the chunk followed by "data")
    subchunk2SizePosition = inputFile.tellg();
    std::cout << "Data starts at is at: \t\t" << subchunk2SizePosition << std::endl;
    return subchunk2SizePosition;
}

int readFile(std::string fileName, Wave& wave)
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

        //Move to the data chunk and read it
        int subchunk2SizePosition = findData(inputFile,36);

        //Create a buffer to read the data size chunk
        char dataSizeBuffer[4];
        //Read the data chunk following "data"
        inputFile.seekg(subchunk2SizePosition, std::ios::beg);
        inputFile.read(dataSizeBuffer, sizeof(dataSizeBuffer));
        //Convert the data chunks value to an integer
        wave.subchunk2_size = *reinterpret_cast<int*>(&dataSizeBuffer);
        //If that integer is 0, the file is messed up and probably has a data chunk elsewhere as seen in audio2.wav
        while (wave.subchunk2_size == 0)
        {
            subchunk2SizePosition = findData(inputFile, subchunk2SizePosition);
            inputFile.seekg(subchunk2SizePosition, std::ios::beg);
            inputFile.read(dataSizeBuffer, sizeof(dataSizeBuffer));
            wave.subchunk2_size = *reinterpret_cast<unsigned int*>(&dataSizeBuffer);
        }
        
        wave.number_of_samples = wave.subchunk2_size / (wave.num_channels * (wave.bits_per_sample / 8));
        wave.duration = wave.number_of_samples / wave.sample_rate;

        //Read every 3rd byte
        int counter = 0;
        char bytes[4];
        std::vector<short> audioData;
        inputFile.seekg(subchunk2SizePosition, std::ios::beg);

        //Every 2 samples is 4 bytes. Read every n bytes where n is a multiple of 4 (skip some bytes for the sake of performance in ImPlot)
        int n = 4;
        if (n % 4 != 0)
            n = 1;

        while (inputFile.read(bytes, sizeof(bytes))) 
        {
            //Read only every nth set of 4 bytes (Read every other nth sample)
            if (counter % n == 0) 
            {
                amplitude_vector_channel1.push_back((float)(*reinterpret_cast<short*>(&bytes[0])));
                amplitude_vector_channel2.push_back((float)(*reinterpret_cast<short*>(&bytes[2])));
                audio_time.push_back(counter/wave.duration);
            }
            counter++;
        }

    }
    else {
        std::cerr << "Error: Unable to open the file: " << fileName << std::endl;
        return -1;
    }
    inputFile.close();
    return 0;
}

static void HelpMarker(const char* desc)
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
    //Graphical User Interface
    setup();

    bool isFileOpen = false;
    static char file_name_buffer[128] = "test samples/Q1/";
    Wave wave;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        //Reset viewport
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
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
            ImGui::Begin("Wave Form");

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
            ImGui::End();

            //Set partner window size and position
            ImGui::SetNextWindowSize(ImVec2((displayX * 2  * 0.10), displayY), ImGuiCond_Once);
            ImGui::SetNextWindowPos(ImVec2(displayX, 0), ImGuiCond_Once);

            //Display file properties in a partner window
            ImGui::Begin("Properties");
            ImGui::Text("Subchunk1 Size: \t\t\t%i", wave.subchunk1_size);
            ImGui::Text("Audio Format:\t\t\t\t%i", wave.audio_format);
            ImGui::Text("Number of Channels:  \t\t%i", wave.num_channels);
            ImGui::Text("Sample Rate:  \t\t\t%i", wave.sample_rate);
            ImGui::Text("Byte Rate:   \t\t\t%i", wave.byte_rate);
            ImGui::Text("Block Align: \t\t\t\t%i", wave.block_align);
            ImGui::Text("Bits Per Sample:\t\t\t%i", wave.bits_per_sample);
            ImGui::Text("Number of Samples:   \t%i", wave.number_of_samples);
            ImGui::Text("Duration (s):\t\t\t\t%i", wave.duration);
            ImGui::Spacing();
            ImGui::Text("\nINFO:\n");
            ImGui::Text("\nClick on wave form and drag to move.\n\nUse scroll wheel to adjust zoom.");

            ImGui::End();

        }
        //Input File Window
        else
        {
            //Set input file window size and position
            ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Once);
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));

            //Display input file window
            ImGui::Begin("InputWindow");

            ImGui::Text("File Location");
            ImGui::InputText("##File Location: ", file_name_buffer, 128);

            ImGui::SameLine(); HelpMarker(
                "Path relative to solution directory.\n");
            ImGui::Spacing();

            if (ImGui::Button("Submit")) {
                if (readFile(file_name_buffer, wave) == 0)   {
                    isFileOpen = true;
                }
            }

            //Some shortcuts for easier testing
            ImGui::Spacing();
            ImGui::Text("Shortcuts");
            if (ImGui::Button("audio1.wav")){
                if (readFile("test samples/Q1/audio1.wav", wave) == 0) {
                    isFileOpen = true;
                }
            }

            if (ImGui::Button("audio2.wav"))
            {
                if (readFile("test samples/Q1/audio2.wav", wave) == 0) {
                    isFileOpen = true;
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