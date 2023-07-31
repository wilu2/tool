import os
import sys
import json
import re


def clean():
    test_set_path = r'/data/vincent_xie/contract/test_set'
    test_set_file = os.path.join(test_set_path, "test_set.json")
    
    with open(test_set_file, 'r', encoding='utf-8') as f:
        test_set_filelist = json.load(f)

    doc_files = []
    doc_files.append(test_set_file)

    for doc_pair in test_set_filelist:
        file1 = os.path.join(test_set_path, doc_pair[0])
        file2 = os.path.join(test_set_path, doc_pair[1])
        doc_files.append(file1)
        doc_files.append(file2)
        
    
    for root,dirs,files in os.walk(test_set_path):
        for filename in files:
            fn_name = os.path.join(root, filename)
            if fn_name not in doc_files:
                print(fn_name)
                os.unlink(fn_name)
                





if __name__ == '__main__':
    # clean()
    # exit(-1)
    
    
    test_set_path = r'/home/xin_yang/contract/test_set'
    test_set_file = os.path.join(test_set_path, "test_set.json")
    
    with open(test_set_file, 'r', encoding='utf-8') as f:
        test_set_filelist = json.load(f)

    for doc_pair in test_set_filelist:
        print(doc_pair)
        cmd = './build/demo/demo {} {}'.format(os.path.join(test_set_path, doc_pair[0]), os.path.join(test_set_path, doc_pair[1]))
        res = os.popen(cmd)
        print(res.read())