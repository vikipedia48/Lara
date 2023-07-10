#ifndef PREVIEWTASK_H
#define PREVIEWTASK_H

#include <CImg.h>
#include <QRunnable>



template <typename T> class PreviewTask : public QRunnable
{
public:
    cimg_library::CImg<T> img;
    PreviewTask(const cimg_library::CImg<T>& img) : img(img) {}

public:
    void run() override {
        img.display();
    }
};

#endif // PREVIEWTASK_H
