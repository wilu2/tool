#include "contract_compare.h"
#include <iostream>
#include <fstream>
#include <regex>
#include "nlohmann/json.hpp"
#include "diff.h"
#include <dirent.h>
#include <vector>
#include <locale>
#include <codecvt>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <experimental/filesystem>
#include <regex>
#include "nlohmann/json.hpp"
#include "httplib/httplib.h"
#include "yaml-cpp/yaml.h"
#include "md5.h"
#include "base64.h"
//#include "pdf2img.h"
//#include "word2img.h"
#include "xie_filesystem.h"
#include "str_utils.h"
#include "xie_debug.h"

using namespace std;

namespace fs = std::experimental::filesystem;

// 图片目录
static std::string g_scan_path;

// 文件名匹配
static std::string g_file_regex;


std::string g_root;


struct PostParam 
{
    std::string host;
    int port;
    std::string path;

    bool is_valid()
    {
        return (host.size() && port > 0 && path.size());
    }
};

static PostParam g_ocr_param;


void loadConfig(const std::string &filename)
{
    YAML::Node config = YAML::LoadFile(filename);
    g_ocr_param.host = config["ocr"]["host"].as<std::string>();
    g_ocr_param.port = config["ocr"]["port"].as<int>();
    g_ocr_param.path = config["ocr"]["path"].as<std::string>();

    g_scan_path = config["scan_path"].as<std::string>();

    if (config["file_regex"])
    {
        g_file_regex = config["file_regex"].as<std::string>();
    }
}


string global_file_name;

string recogSlices(void* p, cv::Mat& image, string type,
                   const vector<int>& slice_vec,
                   map<string, string> map_recog_params) 
{
#if 1
  bool need_red = false;
  if (map_recog_params.find("color_channel") != map_recog_params.end() &
      map_recog_params["color_channel"] == "red") {
    need_red = true;
  }
  cv::Mat painter = image.clone();
  int width = int(painter.cols);
  int height = int(painter.rows);

  vector<vector<int> > slices;

  for (int i = 0; i < slice_vec.size(); i += 4) {
    vector<int> tmp_sl = {slice_vec[i + 0], slice_vec[i + 1], slice_vec[i + 2],
                          slice_vec[i + 3]};
    slices.push_back(tmp_sl);
    cv::rectangle(painter, cv::Point(slice_vec[i + 0], slice_vec[i + 1]),
                  cv::Point(slice_vec[i + 2], slice_vec[i + 3]),
                  cv::Scalar(255), 2, 2);
  }

  cv::Mat tmp = painter(cv::Rect(cv::Point(0, 0), cv::Point(width, height)));

  if (need_red) {
    cv::Mat rgbChannels[3];
    cv::split(tmp, rgbChannels);
    cv::Mat blue = rgbChannels[2];
    cv::cvtColor(blue, tmp, cv::COLOR_GRAY2BGR);
  }

  // cv::namedWindow("slices", CV_WINDOW_NORMAL);
  // cv::imshow("slices", tmp);
  // cv::waitKey(0);
  // cv::imwrite("./tmp.jpg", tmp);

  vector<uchar> buf;
  cv::imencode(".jpg", tmp, buf);
  string imgStr = xwb::base64_encode(buf.data(), buf.size());
 

  nlohmann::json slisJson;
  slisJson["image"] = imgStr;
  slisJson["slices"] = slices;
  string body = slisJson.dump();
  std::cout << "httplib::Client" << std::endl;
// cout << body << endl;

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  httplib::SSLClient cli("ai.intsig.net", 443);
#else
  httplib::Client cli("sh-imgs-sandbox.intsig.net");
  // httplib::Client cli("192.168.16.117", 4248);
#endif

  std::cout << " cli "
            << "body length" << body.length() << std::endl;

  try {
    // auto res = cli.Post("/icr/ctpn_recognize_slices_no_detect", body,
    // "json");
    auto res = cli.Post("/1026/icr3/ctpn_recognize_slices", body, "json");
    std::cout << " recog resp " << res->status << std::endl;
    if (res->status != 200) {
      return "-1";
    }

    std::string json = res->body;
    return json;

  } catch (exception& e) {

    cout << e.what() << endl;

  }

#endif

  return "-1";

}

std::string api_file_type;
std::string recognizeBufByAPIFormData(unsigned char *buf, int size, PostParam& param)
{
    std::string body((char *)buf, size);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(param.host.c_str(), param.port);
#else
    httplib::Client cli(param.host.c_str(), param.port);
#endif

    httplib::MultipartFormDataItems formdatas;
    httplib::MultipartFormData formdata;

    formdata.name = "file";
    formdata.content = body;
    formdata.filename = api_file_type;
    formdatas.push_back(formdata);

    {
        httplib::MultipartFormData fd;
        fd.name = "engine";
        fd.content = "table";
        formdatas.push_back(fd);
    }

    {
         httplib::MultipartFormData fd;
         fd.name = "merge_same_row";
         fd.content = "true";
         formdatas.push_back(fd);
    }

    {
         httplib::MultipartFormData fd;
         fd.name = "apply_stamp";
         fd.content = "1";
         formdatas.push_back(fd);
    }
    //     {
    //      httplib::MultipartFormData fd;
    //      fd.name = "remove_stamp";
    //      fd.content = "true";
    //      formdatas.push_back(fd);
    // }
    {
         httplib::MultipartFormData fd;
         fd.name = "use_pdf_parser";
         fd.content = "true";
         formdatas.push_back(fd);
    }
    {
         httplib::MultipartFormData fd;
         fd.name = "remove_footnote";
         fd.content = "false";
         formdatas.push_back(fd);
    }
{
         httplib::MultipartFormData fd;
         fd.name = "remove_annot";
         fd.content = "false";
         formdatas.push_back(fd);
    }
    {
         httplib::MultipartFormData fd;
         fd.name = "remove_edge";
         fd.content = "false";
         formdatas.push_back(fd);
    }
    {
         httplib::MultipartFormData fd;
         fd.name = "return_image";
         fd.content = "true";
         formdatas.push_back(fd);
    }
    // {
    //      httplib::MultipartFormData fd;
    //      fd.name = "image_scale";
    //      fd.content = "2";
    //      formdatas.push_back(fd);
    // }

    std::cout << " cli "
              << "body length" << body.length() << std::endl;
    try
    {
        auto res = cli.Post(param.path.c_str(), formdatas);
        std::string json = res->body;
        return json;
    }
    catch (exception &e)
    {
        cout << e.what() << endl;
    }
    return "";
}


std::string recognizeBufByAPI(unsigned char *buf, int size, PostParam& param)
{
    return recognizeBufByAPIFormData(buf, size, param);


    std::string body((char *)buf, size);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli(param.host.c_str(), param.port);
#else
    httplib::Client cli(param.host.c_str(), param.port);
#endif

    std::cout << " cli "
              << "body length" << body.length() << std::endl;
    try
    {
        auto res = cli.Post(param.path.c_str(), body, "image/*");
        std::string json = res->body;
        return json;
    }
    catch (exception &e)
    {
        cout << e.what() << endl;
    }
}

std::string recognizeBuf(unsigned char *buf, int size, std::string raw_dir)
{
    std::string md5_str = xwb::MD5Digest(buf, size);

    std::string filename = raw_dir + "/" + md5_str + ".json";
    cout << filename << endl;
    std::string ocr_result;
    if(fs::exists(filename))
    {
        std::string line_str;
        std::ifstream ifs(filename, std::ifstream::in);
        nlohmann::json j;
        ifs >> j;
        ocr_result = j.dump();
        ifs.close();
    }
    else
    {
        ocr_result = recognizeBufByAPI(buf, size, g_ocr_param);
        ofstream ofs(filename, std::ofstream::out);
        ofs << ocr_result;
        ofs.close();
    }
    return ocr_result;
}

std::string recognizeMat(const cv::Mat& image, std::string raw_dir)
{
    vector<uchar> vbuf;
    cv::imencode(".jpg", image, vbuf);
    return recognizeBuf(vbuf.data(), vbuf.size(), raw_dir);
}

std::string recognizeFile(const std::string& filename, std::string raw_dir)
{
    smatch m;
    if(regex_search(filename, m, regex(".(docx?)$")))
    {
        api_file_type = "02.docx";
    }
    else if(regex_search(filename, m, regex(".(pdf)$")))
    {
        api_file_type = "02.pdf";
    }
    else if(regex_search(filename, m, regex(".(xlsx)$")))
    {
        api_file_type = "02.xlsx";
    }
    else 
    {
        api_file_type= "02.jpg";
    }



    FILE *fp = fopen(filename.c_str(), "rb");
    vector<unsigned char> buf;
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        buf.resize(ftell(fp));
        fseek(fp, 0, SEEK_SET);
        fread(buf.data(), buf.size(), 1, fp);
        fclose(fp);
    }
    //cout << "start" << endl;
    std::string ocrResult = recognizeBuf(buf.data(), buf.size(), raw_dir);
    //cout << ocrResult << endl;
    return ocrResult;
}



void ReadFolder(const std::string folder, std::vector<std::string> &lines,

                std::vector<std::string> &file_name)
{
    if (folder == "")
    {
        std::cout << "folder name invalid, plz check." << std::endl;
        return;
    }

    DIR *pDir = opendir(folder.c_str());
    if (NULL == pDir)
    {
        std::cout << folder << " don't exist." << std::endl;
        return;
    }
    std::string prefix_name = folder;
    dirent *pDRent = NULL;
    while ((pDRent = readdir(pDir)) != NULL)
    {
        if (strlen(pDRent->d_name) > 3)
        {
            std::string postfix_name(pDRent->d_name);
            file_name.push_back(postfix_name);
            std::string full_path = prefix_name + "/" + postfix_name;
            lines.push_back(full_path);
        }
    }
    closedir(pDir);
}



bool newfolders(const std::string& foldername)
{
    if(!fs::exists(foldername))
        return fs::create_directories(foldername);
    return true;
}


void persistenceJson(const std::string& filename, const std::string& strucured_result)
{
    std::ofstream os(filename, std::ofstream::out);
    os << strucured_result;
    os.close();
}





int test_engine(const std::string& scan_path, void *pContext)
{
    std::string d_img = scan_path;
    std::string d_result_json = d_img +  "/result";
    std::string d_result_txt = d_img + "/result_txt";

    std::string d_ocr = d_img + "/raw_output";

    std::string d_dst_img = d_img + "/img_dir";

    std::string d_meta = d_img + "/meta";



    newfolders(d_result_json);

    newfolders(d_result_txt);

    newfolders(d_ocr);

    newfolders(d_dst_img);

    newfolders(d_meta);



    std::vector<std::string> file_path;

    std::vector<std::string> file_name;

    ReadFolder(d_img, file_path, file_name);



    // std::sort(file_path.begin(), file_path.end(), [](std::string& a, std::string& b){

    //       string str1 = a.substr(a.find_last_of('/')+1);

    //       string str2 = b.substr(b.find_last_of('/')+1);



    //       if(regex_search(str1, regex("^[0-9].*")) && regex_search(str2, regex("^[0-9].*")))

    //       {

    //           return std::stoi(str1) < std::stoi(str2);

    //       }

    //       return str1 < str2;

    // });





    for (auto& full_file_name : file_path)

    {

        if (!fs::is_regular_file(full_file_name))

            continue;



        if (g_file_regex.size() && !regex_search(full_file_name, regex(g_file_regex)))

        {

            continue;

        }



        string filename = fs::path(full_file_name).filename().generic_string();

        string pure_filename = fs::path(filename).replace_extension("").generic_string();



        cv::Mat image = cv::imread(full_file_name, 129);

        if (image.empty())

            continue;



        

        

        std::string fn_result_json = d_result_json + "/" + pure_filename + ".json";

        std::string fn_result_txt = d_result_txt + "/" + pure_filename + ".txt";

        std::string fn_dst_img = d_dst_img + "/" + filename;

        std::string fn_meta = d_meta + "/" + pure_filename + ".json";

        

        cout << "full_file_name:" << full_file_name << endl;



        std::string ocr_result = recognizeFile(full_file_name, d_ocr);



        if (ocr_result == "-1")

        {

            std::cout << "ocr fail!" << std::endl;

            continue;

        }



        cout << fn_meta << endl;

        cout << fn_result_json << endl;

        cout << fn_result_txt << endl;

    }
    return 0;
}




int render(const std::string& filename, std::vector<cv::Mat>& images)
{
    if(std::regex_search(filename, std::regex("(pdf)$")))
    {
        //xwb::PDF2Img pdf2img;
        //pdf2img.convert(filename, images);
    }
    else if(std::regex_search(filename, std::regex("docx?$")))
    {
        //xwb::Word2Img word2img;
        //word2img.convert(filename, images);
    }
    else
    {
        cv::Mat image = cv::imread(filename, 1);
        images.push_back(image);
    }
    return 0;
}

int load_images(const std::string& folder, std::vector<cv::Mat>& images)
{
    int count = xwb::fs::countFiles(folder);

    for(int i = 0; i < count; i++)
    {
        std::string filename = folder + "/" + std::to_string(i) + ".jpg";

        if(xwb::fs::exist(filename))
        {
            cv::Mat image = cv::imread(filename, 1);
            images.push_back(image);
        }
    }

    return images.size();
}

std::string get_dir(const std::string& filename)
{
    std::experimental::filesystem::path p(filename);
    return p.parent_path().generic_string();
}

std::string renderOutputPath(const std::string& filename)
{
    std::string dir_name = get_dir(filename);
    std::string md5_str = xwb::fs::fileMD5(filename);
    std::string output = dir_name + "/" + md5_str;

    std::cout << "filename: " << filename << endl;
    std::cout << "savepath: " << output << endl;

    return output;
}

int save_images(const std::string& dir_name, std::vector<cv::Mat>& images)
{
    newfolders(dir_name);
    for(int i = 0; i < images.size(); i++)
    {
        std::string filename = dir_name + "/" + std::to_string(i) + ".jpg";
        cv::imwrite(filename, images[i]);
    }
    return 0;
}


int render_or_load(const std::string& filename, std::vector<cv::Mat>& images)
{
    std::string save_path = renderOutputPath(filename);

    if(!xwb::fs::exist(save_path))
    {
        render(filename, images);
        save_images(save_path, images);
        images.clear();
    }
    
    load_images(save_path, images);
    return images.size();
}   







std::string recognize(cv::Mat& image)
{
    const std::string raw_dir = "/home/vincent/workspace/contract_compare/test-images/01/raw_output";
    newfolders(raw_dir);

    std::string ocr_result = recognizeMat(image, raw_dir);
    return ocr_result;
}

int batch_recognize(std::vector<cv::Mat>& images, std::vector<std::string>& ocr_results)
{
    for(auto& img : images)
    {
        std::string ocr_result = recognize(img);
        ocr_results.push_back(ocr_result);
    }
    return 0;
}


std::string concatOCRResult(const std::string& result1, const std::string& result2)
{
    nlohmann::json final_js = nlohmann::json::array();

    nlohmann::json doc1_js;
    doc1_js["doc_index"] = 1;
    doc1_js["pages"] = nlohmann::json::array();

    nlohmann::json page_js1 = nlohmann::json::parse(result1);
    doc1_js["pages"] = page_js1["result"]["pages"];


    nlohmann::json doc2_js;
    doc2_js["doc_index"] = 2;
    doc2_js["pages"] = nlohmann::json::array();

    nlohmann::json page_js2 = nlohmann::json::parse(result2);
    doc2_js["pages"] = page_js2["result"]["pages"];
    
    final_js.push_back(doc1_js);
    final_js.push_back(doc2_js);
    return final_js.dump();
}


std::string concatOCRResult(std::vector<std::string>& result1, std::vector<std::string>& result2)
{
    nlohmann::json final_js = nlohmann::json::array();

    nlohmann::json doc1_js;
    doc1_js["doc_index"] = 1;
    doc1_js["pages"] = nlohmann::json::array();

    for(int i = 0; i < result1.size(); i++)
    {
        nlohmann::json page_js = nlohmann::json::parse(result1[i]);
        page_js["page_index"] = i+1;
        page_js["status"] = 200;
        doc1_js["pages"].push_back(page_js);
    }

    nlohmann::json doc2_js;
    doc2_js["doc_index"] = 2;
    doc2_js["pages"] = nlohmann::json::array();

    for(int i = 0; i < result2.size(); i++)
    {
        nlohmann::json page_js = nlohmann::json::parse(result2[i]);
        page_js["page_index"] = i+1;
        page_js["status"] = 200;
        doc2_js["pages"].push_back(page_js);
    }

    final_js.push_back(doc1_js);
    final_js.push_back(doc2_js);
    return final_js.dump();
}

struct DifItem
{
    std::string text;
    std::string status;

    std::vector<int> polygon;
    std::vector<std::vector<int> > char_polygons;
    int page_index;
    int doc_index;

    DifItem()
        :page_index(0), doc_index(0){}
};

void parseDifItem(nlohmann::json& js, DifItem& item)
{
    item.text = js["text"].get<std::string>();
    item.status = js["status"].get<std::string>();
    item.page_index = js["page_index"].get<int>();
    item.doc_index = js["doc_index"].get<int>();
    item.polygon = js["polygon"].get<std::vector<int> >();
    item.char_polygons = js["char_polygons"].get<std::vector<std::vector<int> > >();
}

cv::Scalar statusColor(const std::string& status)
{
    if(status == "CHANGE")
    {
        return cv::Scalar(255); // 蓝色
    }
    else if(status == "INSERT")
    {
        return cv::Scalar(0, 255); // 绿色
    }
    else if(status == "DELETE") // 蓝色
    {
        return cv::Scalar(0, 0, 255); 
    }

    return cv::Scalar(128,128,128);
}

int visualResultDiff(const std::string& result, std::vector<cv::Mat> images1, std::vector<cv::Mat> images2)
{
    nlohmann::json js = nlohmann::json::parse(result);
    vector<float> rate1;
    vector<float> rate2;
    for(auto& image : images1)
    {
        float rate = 800.0 / image.cols;
        cv::resize(image, image, cv::Size(800, 800*image.rows/image.cols));
        rate1.push_back(rate);
    }

    for(auto& image : images2)
    {
        float rate = 800.0 / image.cols;
        cv::resize(image, image, cv::Size(800, 800*image.rows/image.cols));
        rate2.push_back(rate);
    }

    for(auto& item : js["result"]["detail"])
    {
        std::string status = item["status"].get<std::string>();
        std::string type = item["type"].get<std::string>();
        cv::Scalar color = statusColor(status);

        if(type == "text" || type == "stamp")
        {
            for(auto& c : item["diff"][0])
            {
                vector<int> polygon = c["polygon"].get<std::vector<int> >();
                int page_index = c["page_index"].get<int>();

                float rate = 1.0;
                if(rate1.size() > 0)
                    rate = rate1[page_index];
                if(images1.size() > 0)    
                    cv::rectangle(images1[page_index], 
                                cv::Point(polygon[0] * rate, polygon[1] * rate),
                                cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                color, 2);
            }
            for(auto& c : item["diff"][1])
            {
                vector<int> polygon = c["polygon"].get<std::vector<int> >();
                int page_index = c["page_index"].get<int>();
        
                float rate = 1.0;
                if(rate2.size() > 0)
                    rate = rate2[page_index];
                if(images2.size() > 0)
                    cv::rectangle(images2[page_index], 
                                cv::Point(polygon[0] * rate, polygon[1] * rate),
                                cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                color, 2);
            }
        }
        else if(type == "table" || type == "row" || type == "cell")
        {
            {
                vector<int> polygon = item["area"][0]["polygon"].get<std::vector<int> >();
                int page_index = item["area"][0]["page_index"].get<int>();
                float rate = 1.0;
                if(rate1.size() > 0)
                    rate = rate1[page_index];
                if(images1.size() > 0)
                    cv::rectangle(images1[page_index], 
                                cv::Point(polygon[0] * rate, polygon[1] * rate),
                                cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                color, 2);
            }

            {
                vector<int> polygon = item["area"][1]["polygon"].get<std::vector<int> >();
                int page_index = item["area"][1]["page_index"].get<int>();
                float rate = 1.0;
                if(rate1.size() > 0)
                    rate = rate1[page_index];
                if(images2.size() > 0)
                    cv::rectangle(images2[page_index], 
                                cv::Point(polygon[0] * rate, polygon[1] * rate),
                                cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                color, 2);
            }



            for(auto& d : item["diff"])
            {
                std::string status = d["status"].get<std::string>();
                cv::Scalar sub_color = statusColor(status);

                for(auto& c : d["diff"][0])
                {
                    vector<int> polygon = c["polygon"].get<std::vector<int> >();
                    int page_index = c["page_index"].get<int>();
                    float rate = 1.0;
                    if(rate1.size() > 0)
                        rate = rate1[page_index];
                    if(images1.size() > 0)
                        cv::rectangle(images1[page_index], 
                                    cv::Point(polygon[0] * rate, polygon[1] * rate),
                                    cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                    sub_color, 2);
                }
                for(auto& c : d["diff"][1])
                {
                    vector<int> polygon = c["polygon"].get<std::vector<int> >();
                    int page_index = c["page_index"].get<int>();
                    float rate = 1.0;
                    if(rate2.size() > 0)
                        rate = rate2[page_index];
                    if(images2.size() > 0)
                        cv::rectangle(images2[page_index], 
                                    cv::Point(polygon[0] * rate, polygon[1] * rate),
                                    cv::Point((polygon[4]+2) * rate, polygon[5] * rate),
                                    sub_color, 2);
                }   
            }
        }
    }

    std::string save_path = g_root + "/visual_result/";
    newfolders(save_path);

    for(int i = 0; i < images1.size(); i++)
    {
        std::string filename = save_path + std::to_string(0) + "_" + std::to_string(i) + ".jpg";
        //cout << filename << endl;
        cv::imwrite(filename, images1[i]);
    }

    for(int i = 0; i < images2.size(); i++)
    {
        std::string filename = save_path + std::to_string(1) + "_" + std::to_string(i) + ".jpg";
        //cout << filename << endl;
        cv::imwrite(filename, images2[i]);
    }
    return 0;
}

int visualResult(const std::string& result, std::vector<cv::Mat> images1, std::vector<cv::Mat> images2)
{
    nlohmann::json js = nlohmann::json::parse(result);

    std::unordered_map<int, cv::Mat> m;


    for(auto& item : js["result"]["detail"])
    {
        DifItem item1;
        DifItem item2;

        parseDifItem(item[0], item1);
        parseDifItem(item[1], item2);

        if(item1.page_index == -1 || item2.page_index == -1)
        {
            item1.page_index = max(item1.page_index, item2.page_index);
            item2.page_index = max(item1.page_index, item2.page_index);
        }

        int key = (item1.page_index << 16) + item2.page_index;

        cv::Mat& img1 = images1[item1.page_index];
        cv::Mat& img2 = images2[item2.page_index];


        
        cv::Mat post_img1;
        cv::resize(img1, post_img1, cv::Size(800, 800*img1.rows/img1.cols));

        cv::Mat post_img2;
        cv::resize(img2, post_img2, cv::Size(800, 800*img2.rows/img2.cols));

        if(!m.count(key))
        {
            cv::Mat painter(max(post_img1.rows, post_img2.rows), post_img1.cols+post_img2.cols, CV_8UC3);
            post_img1.copyTo(painter(cv::Rect(0, 0, post_img1.cols, post_img1.rows)));
            post_img2.copyTo(painter(cv::Rect(post_img1.cols, 0, post_img2.cols, post_img2.rows)));
            m[key] = painter;
        }

        cv::Mat& painter = m[key];

        cv::Scalar color = statusColor(item1.status);

        //if(item1.text.size())
        for(auto& polygon : item1.char_polygons)
        {
            float rate = (float)post_img1.cols / img1.cols;
            rate *= 2;
            cv::rectangle(painter, 
                cv::Point(polygon[0] * rate, polygon[1] * rate),
                cv::Point((polygon[4]+2) * rate, polygon[5] * rate), 
                color, 2);
        }
        
        //if(item2.text.size())
        for(auto& polygon : item2.char_polygons)
        {
            float rate = (float)post_img2.cols / img2.cols;
            rate *= 2;
            cv::rectangle(painter, 
                cv::Point(post_img1.cols + polygon[0]*rate, polygon[1]*rate),
                cv::Point(post_img1.cols+ (polygon[4]+2)*rate, polygon[5]*rate), 
                color,2);
        }
    }

    std::string save_path = g_root + "/visual_result/";
    newfolders(save_path);

    for(auto it = m.begin(); it!= m.end(); it++)
    {
        int k = it->first;
        cv::Mat image = it->second;

        int i = k >>16;
        int j = k & 0xffff;

        std::string filename = save_path + std::to_string(i) + "_" + std::to_string(j) + ".jpg";
        cout << filename << endl;
        cv::imwrite(filename, image);
    }
    return 0;
}


int visual_recognize(const std::string& index_str, const cv::Mat& image, const std::string& ocr_result)
{
    cv::Mat painter = image.clone();

    nlohmann::json js = nlohmann::json::parse(ocr_result);

    std::map<std::string, cv::Scalar> area_colors = {
        {"paragraph", cv::Scalar(255)},
        {"list", cv::Scalar(0, 255)},
        {"edge", cv::Scalar(0, 0, 255)},
        {"table", cv::Scalar(255)},
        {"image", cv::Scalar(0, 0, 255)},
        {"stamp", cv::Scalar(0, 0, 255)},
        {"formula", cv::Scalar(0, 255, 0)},
        {"watermark", cv::Scalar(255)},
    };

    for(auto& area_js : js["areas"])
    {
        vector<int> position = area_js["position"].get<std::vector<int> >();
        std::string type = area_js["type"].get<std::string>();
        for(int i = 0; i < 4; i++)
        {
            cv::line(painter, 
                cv::Point(position[(i*2)%8], position[((i*2+1)%8)]), 
                cv::Point(position[(i*2+2)%8], position[((i*2+3)%8)]),
                cv::Scalar(0, 0, 255), 2);
        }
        if(type=="table")
        cv::putText(painter, type, cv::Point(position[4]+5, position[5]), cv::FONT_HERSHEY_SIMPLEX, 2.0, cv::Scalar(255));
    }

    for(auto& line_js : js["lines"])
    {
        vector<int> position = line_js["position"].get<std::vector<int> >();
        //std::string type = area_js["type"].get<std::string>();
        // for(int i = 0; i < 4; i++)
        // {
        //     cv::line(painter, 
        //         2*cv::Point(position[(i*2)%8], position[((i*2+1)%8)]), 
        //         2*cv::Point(position[(i*2+2)%8], position[((i*2+3)%8)]),
        //         area_colors["paragraph"], 2);
        // }

        //cv::putText(painter, type, cv::Point(position[4]+5, position[5]), cv::FONT_HERSHEY_SIMPLEX, 2.0, cv::Scalar(255));
    }


    std::string filename = g_root + "/visual_ocr_result";
    newfolders(filename);
    filename += "/" + index_str + ".jpg";

    cv::imwrite(filename, painter);
    return 0;
}

int batch_visual_recognize(const std::vector<cv::Mat>& images, std::vector<std::string>& ocr_results)
{
    static int index = 1;
    for(int i = 0; i < images.size(); i++)
    {
        std::string index_str = std::to_string(index) + "_" + std::to_string(i);
        visual_recognize(index_str, images[i], ocr_results[i]);
    }
    index++;
    return 0;
}

int batch_visual_recognize(const std::vector<cv::Mat>& images, std::string& ocr_result)
{
    nlohmann::json js = nlohmann::json::parse(ocr_result);

    static int index = 0;
    for(int i = 0; i < images.size(); i++)
    {
        std::string index_str = std::to_string(index) + "_" + std::to_string(i);
        std::string one_page_ocr_result = js["result"]["pages"][i].dump();
        //cout << one_page_ocr_result << endl;
        visual_recognize(index_str, images[i], one_page_ocr_result);
    }
    index++;
    return 0;
}


int debug_contract_compare(const std::string& filename1, const std::string& filename2)
{
    std::vector<cv::Mat> images1;
    std::vector<cv::Mat> images2;

    render_or_load(filename1, images1);
    render_or_load(filename2, images2);

    std::vector<std::string> ocr_results1;
    std::vector<std::string> ocr_results2;

    batch_recognize(images1, ocr_results1);
    batch_recognize(images2, ocr_results2);

    batch_visual_recognize(images1, ocr_results1);
    batch_visual_recognize(images2, ocr_results2);


    std::string allOcrResult = concatOCRResult(ocr_results1, ocr_results2);

    xwb::RuntimeConfig config;
    std::string result;
    {
        TIME_COST_FUNCTION;
        result  = xwb::contract_compare(allOcrResult, config);
    }
    

    persistenceJson("input.json", allOcrResult);
    persistenceJson("output.json", result);

    visualResult(result, images1, images2);

    return 0;
}


std::string recognize_document(const std::string& filename)
{
    std::string save_dir = g_root + "/raw_output/";
    newfolders(save_dir);
    std::string ocr_result = recognizeFile(filename, save_dir);

    return ocr_result;
}

int resize_image_base_ocr_result(std::string ocr_result, vector<cv::Mat>& images)
{
    nlohmann::json js = nlohmann::json::parse(ocr_result);

    for(int i = 0; i < js["result"]["pages"].size(); i++)
    {
        nlohmann::json js_page = js["result"]["pages"][i];
        int dst_width = js_page["width"].get<int>();
        int dst_height = js_page["height"].get<int>();
        std::string img_base64_str = js_page["image_bytes"].get<std::string>();
        std::string img_binary_str = xwb::base64_decode(img_base64_str);
        std::vector<uint8_t> img_binary((uint8_t*)img_binary_str.c_str(), (uint8_t*)img_binary_str.c_str() + img_binary_str.size());
        cv::Mat img_page = cv::imdecode(img_binary, 1);
        images.push_back(img_page);
    }
    return 0;
}

int debug_contract_compare_electronic_file(const std::string& filename1, const std::string& filename2)
{
#ifdef CV_DEBUG
    std::vector<cv::Mat> images1;
    std::vector<cv::Mat> images2;

    // render_or_load(filename1, images1);
    // render_or_load(filename2, images2);
#endif
    std::string ocr_result1 = recognize_document(filename1);
    std::string ocr_result2 = recognize_document(filename2);
#ifdef CV_DEBUG
    resize_image_base_ocr_result(ocr_result1, images1);
    resize_image_base_ocr_result(ocr_result2, images2);

    batch_visual_recognize(images1, ocr_result1);
    batch_visual_recognize(images2, ocr_result2);
#endif
    
    std::string allOcrResult = concatOCRResult(ocr_result1, ocr_result2);
    {
        nlohmann::json js1 = nlohmann::json::parse(ocr_result1);
        nlohmann::json js2 = nlohmann::json::parse(ocr_result2);
        cout << "pages: " << js1["result"]["pages"].size() 
             << ", " << js2["result"]["pages"].size() << endl;
    }
    


    persistenceJson(g_root + "/input.json", allOcrResult);

    xwb::RuntimeConfig config;
    config.block_compare = false;
    config.ignore_punctuation = true;
    config.merge_diff = false;
    std::string result;
    {
        TIME_COST_FUNCTION;
        result  = xwb::contract_compare(allOcrResult, config);
    }
    

    
    persistenceJson(g_root + "/output.json", result);
#ifdef CV_DEBUG
    //visualResult(result, images1, images2);
    visualResultDiff(result, images1, images2);
#endif
    return 0;
}

void testInputFile()
{
    std::string filename = "/home/vincent/workspace/contract_compare_ted/test-images/43/xls.json";
    ifstream ifs(filename);
    nlohmann::json js;
    ifs >> js;
    xwb::RuntimeConfig config;
    config.ignore_punctuation = true;
    config.punctuation = "^.,;:。?!､'“”‘’()[]{}《》〈〉—_*×□/~→+-><=•、`|~~¢£¤¥§©«®°±»àáèéìíòó÷ùúāēěīōūǎǐǒǔǘǚǜΔΣΦΩ฿‰₣₤₩₫€₰₱₳₴℃℉≈≠≤≥①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳■▶★☆☑☒✓々「」『』【】〒〖〗〔〕㎡㎥・･…．";
    std::string result  = xwb::contract_compare(js.dump(), config);
    cout << result << endl;
    return;
}

void createDebugFolders()
{
    std::string dir_name = g_root + "/paragraph_segment/";
    newfolders(dir_name);

    dir_name = g_root + "/semantic_seg/";
    newfolders(dir_name);
}

void test_diff()
{
    // using namespace diff;
    std::wstring a = L"金额：10.0"
"一、合同标的"
"货物名称、具体规格型号、数量,见.附表。....."
"kaishi货物dd名称、具体规格型号、数量,见.yyy附表。.....";
        std::wstring b = L"金额：1.0"
"一、合同标的"
"货物名称、具体规格型号、数量,见.附表。....."
"kaishi货物xx名称、具体规格型号、数量,见.xxx附表。.....";
    using namespace xwb;
    std::vector<Diff> diffs = diff_main(a, b);
    // vector<Diff> diffs = diff_main(a, b);

    for(auto& d : diffs)
    {
        cout << d.operation << ", " << to_utf8(d.text) << endl;
    }
}



int main(int argc, char **argv)
{
    cout << xwb::get_version() << endl;
    // testInputFile();
    //  return 0;

    loadConfig("/home/xin_yang/contract/contract_compare/demo/runtime_config.yaml");

    std::string fn_doc1 = "/home/xin_yang/contract/contract_compare/test/1.pdf";
    std::string fn_doc2 = "/home/xin_yang/contract/contract_compare/test/2.pdf";

    fn_doc1 = argv[1];
    fn_doc2 = argv[2];

    g_root = get_dir(fn_doc1);
    createDebugFolders();
    
    debug_contract_compare_electronic_file(fn_doc1, fn_doc2);
    cout << "Finished!" << endl;
    return 0;
}