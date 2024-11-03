#include <cstdio>
#include <windows.h>
#include <direct.h>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>

void overlayImages(const cv::Mat &background, const cv::Mat &foreground, cv::Mat &output, int x, int y) {
    cv::Rect roi(x, y, std::min(foreground.cols, background.cols - x), 
    std::min(foreground.rows, background.rows - y));
    if (roi.width <= 0 || roi.height <= 0) {
        output = background.clone();
        return;
    }
    output = background.clone();
    cv::Mat roi_output = output(roi);
    cv::Mat roi_foreground = foreground(cv::Rect(0, 0, roi.width, roi.height));
    for (int i = 0; i < roi.height; ++i) {
        for (int j = 0; j < roi.width; ++j) {
            const cv::Vec4b& pixel_foreground = roi_foreground.at<cv::Vec4b>(i, j);
            cv::Vec4b& pixel_output = roi_output.at<cv::Vec4b>(i, j);
            float alpha_foreground = pixel_foreground[3] / 255.0f;
            float alpha_background = 1.0f - alpha_foreground;
            for (int k = 0; k < 3; ++k) {
                pixel_output[k] = cv::saturate_cast<uchar>(
                    alpha_foreground * pixel_foreground[k] + alpha_background * pixel_output[k]
                );
            }
            pixel_output[3] = cv::saturate_cast<uchar>(
                alpha_foreground * 255 + alpha_background * pixel_output[3]
            );
        }
    }
}

struct png{
    int x, y, type, num;
    std::string name;
}img[999],base[999],face[999],other[999],slip[999];

unsigned int readlsfhelf(std::ifstream &lsffile){
	unsigned short val;
	lsffile.read((char *) &val, 2);
	return val;
}


unsigned int readlsf(std::ifstream &lsffile){
	unsigned val;
	lsffile.read((char *) &val, 4);
	return val;
}

std::string ShiftJISToUTF8(const std::string& shiftjis) {
    int len = MultiByteToWideChar(932,0,shiftjis.c_str(),-1,NULL,0);
    if (len == 0) return "";
    std::wstring wstr(len,0);
    MultiByteToWideChar(932,0,shiftjis.c_str(),-1,&wstr[0],len);
    len = WideCharToMultiByte(CP_UTF8,0,&wstr[0],-1,NULL,0,NULL,NULL);
    if (len == 0) return "";
    std::string utf8(len,0);
    WideCharToMultiByte(CP_UTF8,0,&wstr[0],-1,&utf8[0],len,NULL,NULL);
    return utf8;
}

std::string ShiftJISToGBK(const std::string& shiftjis) {
    int len = MultiByteToWideChar(932, 0, shiftjis.c_str(), -1, NULL, 0);
    if (len == 0) return "";
    std::wstring wstr(len, 0);
    MultiByteToWideChar(932, 0, shiftjis.c_str(), -1, &wstr[0], len);
    len = WideCharToMultiByte(936, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (len == 0) return "";
    std::string gbk(len, 0);
    WideCharToMultiByte(936, 0, wstr.c_str(), -1, &gbk[0], len, NULL, NULL);
    return gbk;
}

int main(int argc,char *argv[]){
    
    const char* input = argv[1];
    std::ifstream lsf(argv[1], std::ios::binary);
    lsf.seekg(10);
    int num = readlsfhelf(lsf);
    lsf.seekg(28);
    int pos = 28;
    int basenum = 0, facenum = 0, othernum = 0, slipnum = 0;
    int len = std::strlen(input);

    for(int i = 0; i < num; i++){
        lsf.seekg(pos);
        char* buffer = new char[len + 1];
        buffer[len] = '\0';
        lsf.read(buffer, len);
        img[i].num = (int(buffer[len - 3]) - '0') * 100 + (int(buffer[len - 2]) - '0') * 10 + (int(buffer[len - 1]) - '0');
        img[i].name = buffer;
        pos += 108;
        pos += len;
        lsf.seekg(pos);
        img[i].x = readlsf(lsf);
        img[i].y = readlsf(lsf);
        lsf.seekg(pos + 24);
        img[i].type = lsf.get();
        pos += 36;
        switch (img[i].type) {
            case 0:
                base[basenum] = img[i];
                basenum++;
            break;
            case 1:
                face[facenum] = img[i];
                facenum++;
            break;
            case 99:
                slip[slipnum] = img[i];
                slipnum++;
            break;
            default:
                other[othernum] = img[i];
                othernum++;
            break;
        }
        delete[] buffer;
        //std::cout<<i<<" "<<img[i].x<<" "<<img[i].y<<" "<<img[i].type<<" "<<img[i].num<<std::endl;
    }
    //std::cout<<basenum<<" "<<facenum<<" "<<othernum<<" "<<slipnum<<std::endl;

    std::string outputpath = "output";
    _mkdir(outputpath.c_str());

    for(int i = 0; i < basenum; i++){
        for(int j = 0; j < facenum; j++){
            int x = abs(base[i].x - face[j].x);
            int y = abs(base[i].y - face[j].y);
            std::string outputname = ".\\" + outputpath + '\\'  + std::to_string(base[i].num) + '_' + std::to_string(face[j].num) + ".png";
            std::string basestring = input;
            basestring[len - 4] = '_';
            basestring[len - 3] = base[i].name[len -3];
            basestring[len - 2] = base[i].name[len -2];
            basestring[len - 1] = base[i].name[len -1];
            basestring += ".png";
            std::string facestring = input;
            facestring[len - 4] = '_';
            facestring[len - 3] = face[j].name[len -3];
            facestring[len - 2] = face[j].name[len -2];
            facestring[len - 1] = face[j].name[len -1];
            facestring += ".png";
            cv::Mat basepng = cv::imread(basestring, cv::IMREAD_UNCHANGED);
            cv::Mat facepng = cv::imread(facestring, cv::IMREAD_UNCHANGED);
            cv::Mat output;
            overlayImages(basepng, facepng, output, x, y);
            imwrite(outputname, output);
            printf("saving...   %d_%d.png\n", base[i].num,face[j].num);
        }
    }
}
