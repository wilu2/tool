#include "pdf2img.h"
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-image.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-embedded-file.h>
#include <poppler/cpp/poppler-toc.h>
#include <iostream>
#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;


// std::string parse(const std::string& filename)
// {
//     string text;
    

//     std::unique_ptr<poppler::document> pdf(poppler::document::load_from_file(filename));

//     if ( pdf )
//     {
//         cout << pdf->create_toc() << endl;
//         std::unique_ptr<poppler::toc> p_toc(pdf->create_toc());

//         poppler::toc_item* p_tem = p_toc->root();

//         vector<poppler::toc_item*> chs= p_tem->children();
//         for(auto item : chs)
//         {
//             cout << item->title().to_utf8().data() << endl;
//         }


//         int pages = pdf->pages();
//         if(pages > 0)
//         {
//             poppler::page::text_layout_enum show_text_layout = poppler::page::physical_layout;
//             std::unique_ptr<poppler::page> p(pdf->create_page(1));
//             auto poppler_str = p->text(poppler::rectf(), show_text_layout);
//             if( poppler_str.size() > 0 )
//             {
//                 text = p->text(poppler::rectf(), show_text_layout).to_utf8().data();
//             }
//         }
//     }
    
//     return text;
// }

// string generate_ctpn_result_from_poppler(const poppler::page * p);




// static bool pdfToMat(const std::string& filename, std::vector<cv::Mat> &images, double res)
// {
//     std::unique_ptr<poppler::document> pdf(poppler::document::load_from_file(filename));
//     if (!pdf)
//     {
//         return false;
//     }

//     int pages_total = pdf->pages();


//     for (int i = 0; i < pages_total; i++)
//     {
//         cv::Mat image;
//         if (!poppler_render_page_res(pdf.get(), i, image, res, res))
//         {
//             continue;
//         }
//         images.push_back(image);
//     }


//     return true;
// }


// static bool load_pdf_multi_page(const std::string& filename, std::vector<cv::Mat> &images, double ppi = 255.0)
// {
//     return pdfToMat(filename, images, ppi);
// }


// void test_render(const std::string& filename)
// {
//     vector<cv::Mat> images;

//     load_pdf_multi_page(filename, images);

//     int i = 0;
//     for(auto& m : images)
//     {
//         cv::imwrite(std::string("./") + std::to_string(i) + ".jpg", m);
//         i++;
//     }
// }


// string generate_ctpn_result_from_poppler(const poppler::page * p)
// {
//     string json = "{}";
//     auto text_list = p->text_list(0);
//     poppler::rectf bbox = p->page_rect();
    

//     for (const poppler::text_box &text : text_list) 
//     {

//         poppler::rectf bbox = text.bbox();
//         poppler::ustring ustr = text.text();
//         string utf8_text  = ustr.to_utf8().data();

//         cout << utf8_text << endl;
//         cout << bbox.left() << ", " << bbox.top() << ", " << bbox.right() << ", " << bbox.bottom() << endl;
//         for(size_t i =0 ;i < ustr.size() ;i++)
//         {
//             poppler::rectf bbox = text.char_bbox(i);
//         }
//     }

//     return json;

// }

// int main(int argc, char *argv[])
// {
//     std::string filename = "/home/vincent/workspace/pdf2text/test_images/sig.pdf";

//     std::string text = parse(filename);


//     //test_render(filename);    

//     //cout << text << endl;
//     return 0;
// }


namespace xwb
{
    

static int poppler_render_page_res(poppler::document& doc, int page_num, cv::Mat &mat, double xres = 144.0, double yres = 144.0)
{
    std::unique_ptr<poppler::page> page(doc.create_page(page_num));

    if (!page)
    {
        printf(" get document's page failed\n");
        return false;
    }

    //如果长边是默认的144，就设置适配长边到2048 引擎会缩放到这个尺寸
    if( NULL != page && abs(yres - 144.0) < 0.0001)
    {
        const poppler::page *p = page.get();
        double max = std::max(p->page_rect().width(),  p->page_rect().height());
        if( max >= 2048 )
        {
            xres = 72;
            yres = 72;
        }
        else if( max > 0.0 )
        {
            double  res =  ((double)2048 * 72 ) / max;
            if( res >= 72 && res <= 144)
            {
                xres = res;
                yres = res;
            }
        }
    }


    poppler::page_renderer renderer;
    renderer.set_render_hints(poppler::page_renderer::antialiasing |
                                poppler::page_renderer::text_antialiasing |
                                poppler::page_renderer::text_hinting);

    poppler::image img = renderer.render_page(page.get(), xres, yres);

    int type;
    switch (img.format())
    {
        case poppler::image::format_mono:
            type = CV_8UC1;
            break;
        case poppler::image::format_rgb24:
            type = CV_8UC3;
            break;
        case poppler::image::format_argb32:
            type = CV_8UC4;
            break;
        default:
            printf(" unknown image color format: %d\n", img.format());
            return false;
            break;
    }

    mat = cv::Mat(img.height(), img.width(), type, (void *)img.const_data(), img.bytes_per_row());

    if (type == CV_8UC1)
    {
        cv::cvtColor(mat, mat, cv::COLOR_GRAY2BGR);
    }
    else if (type == CV_8UC4)
    {
        cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
    }
    return true;
}

static int pdfDocToMat(poppler::document& pdf_doc, std::vector<cv::Mat> &images, double res)
{
    int pages_total = pdf_doc.pages();

    for (int i = 0; i < pages_total; i++)
    {
        cv::Mat image;
        if (!poppler_render_page_res(pdf_doc, i, image, res, res))
        {
            continue;
        }
        images.push_back(image);
    }

    return images.size();
}


int PDF2Img::convert(const std::string& filename, std::vector<cv::Mat>& images)
{
    std::unique_ptr<poppler::document> pdf(poppler::document::load_from_file(filename));
    return pdfDocToMat(*pdf.get(), images, 300);
}

int PDF2Img::convert(const std::vector<unsigned char>& file_data, std::vector<cv::Mat>& images)
{
    std::unique_ptr<poppler::document> pdf(poppler::document::load_from_raw_data((char *)file_data.data(), file_data.size()));
    return pdfDocToMat(*pdf.get(), images, 300);
}



} // namespace xwb