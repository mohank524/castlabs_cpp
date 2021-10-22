#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include "base64.h"

#define convert(value) ((0x000000ff & value) << 24) | ((0x0000ff00 & value) << 8) | \
                       ((0x00ff0000 & value) >> 8) | ((0xff000000 & value) >> 24)
					   
#define MAX_READ_SIZE 64*1024 

char* GetTagOrAttribute(char* data, char* start_tag, char* end_tag){
    char* start_pointer = strstr(data, start_tag);
    if(!start_pointer){
        return nullptr;
    }
    long length = strlen(start_tag);
    char* end_pointer = strstr(start_pointer + length, end_tag);
    if(!end_pointer){
        return nullptr;
    }
    
    long size = end_pointer - (start_pointer + length);
    
    char *tag = new char[size + 1];
    strncpy(tag, start_pointer + length, size);
    tag[size] = 0;
    return tag;
}


void ParseImageAndSave(char* data){
    char* ImageName = GetTagOrAttribute(data, (char*)"xml:id=\"", (char*)"\"");
    if(ImageName == nullptr){
        std::cout<< "fail to parse image ImageName" << std::endl;
        return;
    }
 
    char * image_type = GetTagOrAttribute(data, (char *)"imagetype=\"", (char*)"\"");
    if(image_type == nullptr){
        std::cout << "fail to parse image type" << std::endl;
        return;
    }
    
    std::cout << "File ImageName is: " << ImageName << "." << image_type << std::endl;
    
    char * image_body = GetTagOrAttribute(data, (char *)"Base64\">", (char*)"</");
    std::string decoded_body = base64_decode(image_body);
    
    std::string CreateImageFileName = std::string(ImageName) + std::string(".") + image_type;
    
    std::fstream imagefile;
    imagefile.open(CreateImageFileName, std::ios_base::out | std::ios_base::binary);
    
    if(!imagefile.is_open())
	{
        std::cout << "Failed to create new file: " << CreateImageFileName << std::endl;
    }
	else 
	{
        imagefile << decoded_body;
        imagefile.close();
    }
    
    delete [] ImageName;
    delete [] image_type;
    delete [] image_body;
}

void ParseMetaData(char* data, int size){
	
    int pos = 0;
    int image_size = 0;
    char* end = nullptr;
    while(pos < size){
        char* startPoint = strstr(data + pos, "<smpte:image");
        if(!startPoint) 
		{
			std::cout <<"No image found"<<std::endl;
			break;
		}

        end = strstr(data + pos, "</smpte:image>");
		
        if(!end) 
		{
			std::cout <<"Something wrong in xml "<<std::endl;
			break;
		}

        image_size = end - startPoint;
        std::cout << "The image size in XML: " << image_size << " bytes" << std::endl;
		
        ParseImageAndSave(startPoint);
        pos = end - data + std::string("</smpte:image>").size();
    }
}

void ParseBoxInfo(char* data, int size){
    int pos = 0;
    
    uint box_size;
    char box_type[5];
    
    box_type[4] = 0;
    
    while(pos < size){
        box_size = *(uint *)(data + pos);
        pos += 4;

        box_size = convert(box_size);
        
        strncpy(box_type, data + pos, 4);
        pos += 4;
        
        std::cout << "Found box of type " << box_type << " and size " << box_size << std::endl;
        
        if(strcmp((char*)box_type, "traf") == 0){
            ParseBoxInfo(data + pos, box_size - 8);
        }
        
        pos += box_size - 4 - 4;
    }
}


int main(int argc, const char * argv[]) {
    
    std::fstream videoFile;
    uint box_size;
    char box_type[5];
    
    box_type[4] = 0;
    
    if(argc < 2){
        std::cout << "Give File path while executing the program \n";
        return 1;
    }
    
    videoFile.open(argv[1], std::ios_base::in | std::ios_base::binary);
    
    if(!videoFile.is_open()){
        std::cout << "Error opening file: ";
        return 1;
    }

    std::cout << " File has been loaded Successfully";
    std::cout << argv[1] << "\n";

    uint pos = 0;
	videoFile.seekg (0, videoFile.end);
    uint fileLength = videoFile.tellg();
	
    videoFile.seekg (0, videoFile.beg);
	
    std::cout << "File size is: " << fileLength << std::endl;
    

    while(pos < fileLength){
        videoFile.read((char*)&box_size, sizeof(box_size));

        box_size = convert(box_size);
        
        videoFile.read((char*)&box_type, 4);
        
        std::cout << "Found box and type " << box_type << " and size " << box_size << std::endl;
        
        if(strcmp((char*)box_type, "moof") == 0){
            if(box_size < MAX_READ_SIZE){
                char* buffer = new char[box_size];
                if(buffer){
                    for(int i = 0; i <= box_size; i++)
                        videoFile >> buffer[i];

                    ParseBoxInfo(buffer, box_size - 8);
                    
                    delete [] buffer;
                }
            }
        }
        else if(strcmp((char*)box_type, "mdat") == 0){
            if(box_size < MAX_READ_SIZE){
                char* buffer = new char[box_size];
                if(buffer){
                    for(int i = 0; i <= box_size; i++)
                        videoFile >> buffer[i];
                    
                    ParseMetaData(buffer, box_size);

                    std::cout << "Content of mdat block is: ";
                    int count = (box_size < 1024)?box_size:1024;// size should not be excees 1K
                    for(int i = 0; i <= count; i++){
                        std::cout << buffer[i];
                    }
                    delete [] buffer;
                }
            }
        }
        
        pos += box_size;
        videoFile.seekg(pos, std::ios_base::beg);
    }
    
    videoFile.close();
    
    std::cout << std::endl << "We done!" << std::endl;
    return 0;
}