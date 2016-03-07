#include "mainwindow.h"
#include "ui_mainwindow.h"

#define WIDTH 320
#define HEIGHT 240
#define FPS 60

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
//    ui(new Ui::MainWindow),
    _duo(NULL), _dense(NULL)
{
//    ui->setupUi(this);
    resize(320*2, 240);
    setWindowTitle("DUO Test");
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    QSplitter *hs = new QSplitter();
       hs->addWidget(_img[0] = new ImageOutput());
       hs->addWidget(_img[1] = new ImageOutput());
       hs->addWidget(_img[2] = new ImageOutput());
       setCentralWidget(hs);


    DUOResolutionInfo ri;
      if(EnumerateResolutions(&ri, 1, WIDTH, HEIGHT, DUO_BIN_HORIZONTAL2 + DUO_BIN_VERTICAL2, FPS))
      {
          printf("[%dx%d], [%f-%f], %f, [%d]", ri.width, ri.height, ri.minFps, ri.maxFps, ri.fps, ri.binning);

          if(OpenDUO(&_duo)){
              printf("Could not open DUO camera\n");
              close(); // TODO
          }

          char buf[256];
          GetDUOSerialNumber(_duo, buf);
          printf("Serial Number: %s", buf);
          GetDUOFirmwareVersion(_duo, buf);
          printf("Firmware Version: v%s", buf);
          GetDUOFirmwareBuild(_duo, buf);
          printf("Firmware Build Time: %s", buf);
          printf("Library Version: v%s", GetLibVersion());
          printf("Dense3DMT Version:    v%s\n", Dense3DGetLibVersion());
          printf("-------------------------------------------------");

          // Open Dense3D
          if (!Dense3DOpen(&_dense, _duo)) {
              printf("Could not open Dense3DMT library\n");
              close(); // TODO
          }

          // Set the Dense3D license
          if (!SetDense3DLicense(_dense, LICENSE)) // <-- Put your Dense3D license
          {
              printf("Invalid or missing Dense3D license. To get your license visit https://duo3d.com/account\n");
              // Close Dense3D library
              Dense3DClose(_dense);
              close(); // TODO
          }

          // Set the image size
          if (!SetDense3DImageInfo(_dense, WIDTH, HEIGHT, FPS)) {
              printf("Invalid image size\n");
              // Close Dense3D library
              Dense3DClose(_dense);
              close(); // TODO
          }

          // Set Dense3D parameters
          Dense3DParams params;
          params.scale = 0;
          params.mode = 3;
          params.numDisparities = 2;
          params.sadWindowSize = 6;
          params.preFilterCap = 28;
          params.uniqenessRatio = 27;
          params.speckleWindowSize = 52;
          params.speckleRange = 14;
          if (!SetDense3Params(_dense, params)) {
              printf("GetDense3Params error\n");
              // Close Dense3D library
              Dense3DClose(_dense);
              close(); // TODO
          }

          Dense3DStart(_dense, newFrameCb, this);

          SetDUOResolutionInfo(_duo, ri);
          uint32_t w, h;
          GetDUOFrameDimension(_duo, &w, &h);
          qDebug() << "Frame Dimension: [" << w << "," << h << "]";

          SetDUOUndistort(_duo,false);
          SetDUOLedPWM(_duo, 80);
          SetDUOGain(_duo, 50);
          SetDUOExposure(_duo, 50);
          SetDUOVFlip(_duo, false);
      }
  }


//Vec3b HSV2RGB(float hue, float sat, float val) {
//    float x, y, z;

//    if (hue == 1) hue = 0;
//    else hue *= 6;

//    int i = (int) floorf(hue);
//    float f = hue - i;
//    float p = val * (1 - sat);
//    float q = val * (1 - (sat * f));
//    float t = val * (1 - (sat * (1 - f)));

//    switch (i) {
//        case 0:
//            x = val;
//            y = t;
//            z = p;
//            break;
//        case 1:
//            x = q;
//            y = val;
//            z = p;
//            break;
//        case 2:
//            x = p;
//            y = val;
//            z = t;
//            break;
//        case 3:
//            x = p;
//            y = q;
//            z = val;
//            break;
//        case 4:
//            x = t;
//            y = p;
//            z = val;
//            break;
//        case 5:
//            x = val;
//            y = p;
//            z = q;
//            break;
//        default:
//            return Vec3b(0, 0, 0);
//    }
//    return Vec3b((uchar) (x * 255), (uchar) (y * 255), (uchar) (z * 255));
//}

//void prepareColorLut(Mat *colorLut) {
//    for (int i = 0; i < 256; i++) {
//        colorLut->at<Vec3b>(i) = i == 0 ? Vec3b(0, 0, 0) : HSV2RGB(i / 256.0f, 1, 1);
//    }
//}

//Mat colorLut = Mat(cv::Size(256, 1), CV_8UC3);
//prepareColorLut(&colorLut);

void MainWindow::onNewFrame(const PDense3DFrame pFrameData){
    Size frameSize(pFrameData->duoFrame->width,pFrameData->duoFrame->height);

    Mat left(frameSize, CV_8UC1, pFrameData->duoFrame->leftData);
    Mat right(frameSize, CV_8UC1, pFrameData->duoFrame->rightData);
    Mat depth(frameSize, CV_32FC3, pFrameData->depthData);

    cvtColor(left, _leftRGB, COLOR_GRAY2BGR);
    cvtColor(right, _rightRGB, COLOR_GRAY2BGR);
//    cvtColor(depth, _depthRGB, COLOR_GRAY2BGR);

    Q_EMIT _img[0]->setImage(_leftRGB);
    Q_EMIT _img[1]->setImage(_rightRGB);
//    Q_EMIT _img[2]->setImage(depth);
}

MainWindow::~MainWindow()
{
//    delete ui;
    if(_duo) CloseDUO(_duo);
    Dense3DStop(_dense);
    Dense3DClose(_dense);
}

void MainWindow::closeEvent(QCloseEvent *event){

}
