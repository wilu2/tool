#include "word2img.h"
#include <climits>
#include <iostream>
#include <memory>
#include "xie_filesystem.h"
#include "base64.h"


//#include <system/enumerator_adapter.h>
#include <Aspose.Words.Cpp/Model/Text/CommentCollection.h>
#include <Aspose.Words.Cpp/Model/Text/Comment.h>
#include <Aspose.Words.Cpp/Model/Nodes/NodeType.h>
#include <Aspose.Words.Cpp/Model/Nodes/Node.h>
#include <Aspose.Words.Cpp/Model/Document/SaveFormat.h>
#include <Aspose.Words.Cpp/Model/Sections/HeaderFooter.h>
#include <Aspose.Words.Cpp/Model/Nodes/NodeCollection.h>
#include <Aspose.Words.Cpp/Model/Drawing/Shape.h>
#include <Aspose.Words.Cpp/Model/Saving/ImageSaveOptions.h>
#include <Aspose.Words.Cpp/Licensing/License.h>
#include <Aspose.Words.Cpp/Model/Document/Document.h>

#include "thread_pool.hpp"

#include <opencv2/imgcodecs.hpp>

using namespace Aspose::Words;
using namespace Aspose::Words::Drawing;
using namespace Aspose::Words::Saving;

namespace w2i
{


struct RuntimeConfig
{
    int num_threads;
    int resolution;
    bool remove_comments;
    bool remove_watermark;
    bool remove_headerfooter;
    bool remove_footnote;

    RuntimeConfig()
        :num_threads(8), resolution(200), remove_comments(false), remove_watermark(false), remove_headerfooter(false), remove_footnote(false){}
};


int convert_word_to_images_by_aspose(const std::vector<uint8_t> &word_body, std::vector<std::string> &base64_string_vec, RuntimeConfig& config);



int buf_to_file_txt(const char * path , const void * buf, size_t len)
{
    FILE *pf = fopen(path,"wt+");
    if(NULL!= pf)
    {
        fwrite(buf,1,len, pf);
        fclose(pf);
        return 0;
    }
    else
    {
        return -1;
    }
    return 0;
}


const char* base64_encode( const std::vector<unsigned char> &src, std::string &dst)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::stringstream ss;
    unsigned i_bits = 0;
    int i_shift = 0;
    int bytes_remaining = src.size();
    unsigned char *p = (unsigned char *)src.data();
    if (src.size() >= UINT_MAX / 4)
        return NULL;

    while (bytes_remaining) {
        i_bits = (i_bits << 8) + *p++;
        bytes_remaining--;
        i_shift += 8;
        do {
            ss.put(b64[(i_bits << 6 >> i_shift) & 0x3f]);
            i_shift -= 6;
        } while (i_shift > 6 || (bytes_remaining == 0 && i_shift > 0));
    }
    while (ss.tellp() & 3)
        ss.put('=');
    dst = ss.str();
    return dst.data();
}

bool RemoveComments(const System::SharedPtr<Document>& doc)
{
    doc->GetChildNodes(NodeType::Comment, true)->Clear();
    return true;
}

bool RemoveHeaderFooter(const System::SharedPtr<Document>& doc)
{
    doc->GetChildNodes(NodeType::HeaderFooter, true)->Clear();
    return true;
}

bool RemoveFootnote(const System::SharedPtr<Document>& doc)
{
    doc->GetChildNodes(NodeType::Footnote, true)->Clear();
    return true;
}


bool RemoveWatermarkText(const System::SharedPtr<Document>& doc)
{
    System::SharedPtr<NodeCollection> headerFooterNodes = doc->GetChildNodes(NodeType::HeaderFooter, true);
    for (System::SharedPtr<HeaderFooter> hf : System::IterateOver<System::SharedPtr<HeaderFooter>>(headerFooterNodes))
    {
        System::SharedPtr<NodeCollection> shapeNodes = hf->GetChildNodes(NodeType::Shape, true);
        for (System::SharedPtr<Shape> shape: System::IterateOver<System::SharedPtr<Shape>>(shapeNodes))
        {
            if (shape->get_Name().Contains(u"WaterMark"))
            {
                shape->Remove();
            }
        }
    }
    return true;
}


int apply_license_from_file()
{
   System::SharedPtr<License> license = System::MakeObject<License>();

   // This line attempts to set a license from several locations relative to the executable and Aspose.Words.dll.
   // You can also use the additional overload to load a license from a stream, this is useful for instance when the
   // license is stored as an embedded resource
    try
    {
        license->SetLicense(u"Aspose.Words.CPP.lic");
        std::cout << "License set successfully." << std::endl;
    }
    catch (System::Exception& e)
    {
        std::cout << "There was an error setting the license: " << e->get_Message().ToUtf8String() << std::endl;
        std::cout << "retry..." << std::endl;
        try{
            license->SetLicense(u"/home/vincent/ProgramFiles/aspose_dev/Aspose.Words.CPP.lic");
            std::cout << "License set successfully." << std::endl;
        }
        catch(System::Exception& e)
        {
            std::cout << "There was an error setting the license: " << e->get_Message().ToUtf8String() << std::endl;    
        }
    }
    return 0;
}

int get_cpu_count()
{
    int cpu_num = 8; 
    if(std::thread::hardware_concurrency())
    {
        cpu_num = std::thread::hardware_concurrency();
    }
    else if(const char* env_p = std::getenv("OMP_NUM_THREADS")) 
    {
        cpu_num = atoi(env_p);
        cpu_num = std::max(1, std::min(8, cpu_num));        
    }
    return cpu_num;
}

// 文件 t1.doc 有概率crash， 原因未知
std::string convert_one_page(System::SharedPtr<Document> doc, System::SmartPtr<ImageSaveOptions> options, int page_index)
{
    System::SharedPtr<Document> doc_clone = doc->Clone();
    System::SmartPtr<ImageSaveOptions> options_clone = options->Clone();
    options_clone->set_PageIndex(page_index);

    std::string dest;
    auto dstStream = System::MakeObject<System::IO::MemoryStream>();
    doc_clone->Save(dstStream, options_clone);
    base64_encode(dstStream->ToArray()->data(),dest);
    return dest;
}

// 暂时没出现crash
std::string convert_one_page_doc(const std::vector<uint8_t> &word_body, int page_index, RuntimeConfig& config)
{
    System::ArrayPtr<uint8_t> Bytes =  System::MakeArray<uint8_t>(word_body);
    auto byteStream = System::MakeObject<System::IO::MemoryStream>(Bytes);

    auto doc = System::MakeObject<Document>(byteStream);

    config.remove_comments && RemoveComments(doc);
    config.remove_watermark && RemoveWatermarkText(doc);
    config.remove_headerfooter && RemoveHeaderFooter(doc);
    config.remove_footnote && RemoveFootnote(doc);

    auto options = System::MakeObject<ImageSaveOptions>(SaveFormat::Jpeg);
    options->set_PageCount(1);
    options->set_Resolution(config.resolution);
    options->set_PageIndex(page_index);

    std::string dest;
    auto dstStream = System::MakeObject<System::IO::MemoryStream>();
    doc->Save(dstStream, options);
    base64_encode(dstStream->ToArray()->data(),dest);
    return dest;
}

int getPageCount(const std::vector<uint8_t> &word_body)
{
    System::ArrayPtr<uint8_t> Bytes =  System::MakeArray<uint8_t>(word_body);
    auto byteStream = System::MakeObject<System::IO::MemoryStream>(Bytes);
    auto doc = System::MakeObject<Document>(byteStream);
    int page_count = doc->get_PageCount();
    return page_count;
}

int convert_word_to_images_by_aspose_children(const std::vector<uint8_t> &word_body, std::vector<std::string> &base64_string_vec, RuntimeConfig& config)
{
    apply_license_from_file();

    int page_count = getPageCount(word_body);

//#ifdef CV_DEBUG
    std::cout << "get_PageCount: "<< page_count <<std::endl;
//#endif

    std::vector<std::future<std::string> > futures;

    int cpu_num = get_cpu_count();
    cpu_num = std::min(cpu_num, config.num_threads);

    xwb::ThreadPool pool(cpu_num);
    
    for (int i = 0; i < page_count; i++)
    {
        futures.emplace_back(
            pool.enqueue(convert_one_page_doc, word_body, i, config)
        ); 
    }

    // fetch future
    for(size_t i = 0; i<futures.size(); ++i) {
        auto&& fut = futures[i];
        base64_string_vec.push_back(fut.get());
    }

    return 0;
}


int convert_word_to_images_by_aspose(const std::vector<unsigned char> &word_body, 
                                     std::vector<std::string> &base64_string_vec,
                                     RuntimeConfig& config)
{
    try
    {
        convert_word_to_images_by_aspose_children(word_body, base64_string_vec, config);
        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return -1;
}


} //namespace w2i


namespace xwb
{
    




int Word2Img::convert(const std::string& filename, std::vector<cv::Mat>& images)
{
    std::vector<std::string> base64_imgs;
    std::vector<unsigned char> file_data;
    fs::file2Buffer(filename, file_data);

    w2i::RuntimeConfig config;

    int ret = convert_word_to_images_by_aspose(file_data, base64_imgs, config);

    for(auto& base64_img : base64_imgs)
    {
        std::string file_data = base64_decode(base64_img, false);
        std::vector<uchar> file_bin(file_data.begin(), file_data.end());

        cv::Mat image = cv::imdecode(file_bin, 1);
        if(!image.empty())
        {
            images.push_back(image);
        }
    }

    return images.size();
}

int convert(const std::vector<unsigned char>& data, std::vector<cv::Mat>& images)
{
    std::vector<std::string> base64_imgs;
    w2i::RuntimeConfig config;

    int ret = convert_word_to_images_by_aspose(data, base64_imgs, config);

    for(auto& base64_img : base64_imgs)
    {
        std::string file_data = base64_decode(base64_img, false);
        std::vector<uchar> file_bin(file_data.begin(), file_data.end());

        cv::Mat image = cv::imdecode(file_bin, 1);
        if(!image.empty())
        {
            images.push_back(image);
        }
    }

    return images.size();
}


} // namespace xwb
