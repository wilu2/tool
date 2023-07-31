#include "xie_debug.h"

using std::string;

namespace xwb
{
    



string gFilename;
void setDebugFilename(const std::string& filename)
{
    gFilename= filename;
}

std::string getDebugFilename()
{
    return gFilename;
}

std::string getSaveFilename(const std::string& ext)
{
    string savePath("/home/intsig/workspace/barcode/INTSIG_QR/output");
    int p1 = gFilename.find_last_of('/');
    int p2 = gFilename.find_last_of('.');
    return savePath + gFilename.substr(p1, p2-p1+1) + ext;
}


} // namespace xwb