/*
  This example shows how to use Image, Array and Texture to read a .jpg file,
display it as an OpenGL texture and print the pixel values on the command line.
Notice that while the intput image has only 4 pixels, the rendered texture is
smooth.  This is because interpolation is done on the GPU.

  Karl Yerkes and Matt Wright (2011/10/10)
*/

#include "al/core.hpp"
#include "al/util/al_Image.hpp"

#include <cassert>
#include <iostream>
#include <cstdint>
#include <vector>

using namespace al;
using namespace std;

class MyApp : public App {
public:

  // Image and Texture handle reading and displaying image files.
  //
  Image image;
  Texture texture;

  void onCreate() {

    // Load a .jpg file
    //
    const char *filename = "data/image.jpg";

    if (image.load(filename)) {
      printf("Read image from %s\n", filename);
    } else {
      printf("Failed to read image from %s!  Quitting.\n", filename);
      exit(-1);
    }

    cout << "width: " << image.width() << endl;
    cout << "height: " << image.height() << endl;

    image.sendToTexture(texture);
    // or
    // texture.create2D(image.width(), image.height(), Texture::RGB8, Texture::RGB, Texture::UBYTE);
    // texture.submit(image.pixels<uint8_t>(), Texture::RGB, Texture::UBYTE);


    texture.filter(Texture::LINEAR);

    // Here we copy the pixels from the image to the texture
    // texture.allocate(image.array());

    // Don't bother trying to print the image or the image's array directly
    // using C++ syntax.  This won't work:
    //
    //cout << "Image " << image << endl;
    //cout << "   Array: " << image.array() << endl;

    // Make a reference to our image's array so we can just say "array" instead
    // of "image.array()":
    //
    auto& array = image.array();

    // The "components" of the array are like "planes" in Jitter: the number of
    // data elements in each cell.  In our case three components would
    // represent R, G, B.
    //
    cout << "array has " << (int) array.components() << " components" << endl;

    // Each of these data elements is represented by the same numeric type:
    //
    cout << "Array's type (as enum) is " << array.type() << endl;

    // But that type is represented as an enum (see al_Array.h), so if you want
    // to read it use this function:
    //
    printf("Array's type (human readable) is %s\n", allo_type_name(array.type()));

    // The array itself also has a print method:
    //
    cout << "Array.print: "  << endl << "   ";
    array.print();    


    // Code below assumes this type is 8-bit unsigned integer, so this line
    // guarantees that's the case, or else crashes the program if not:
    //
    assert(array.type() == AlloUInt8Ty);

    // AlloCore's image class provides a type for an RGBA pixel, which is of
    // course templated on the numeric type used to represent each value in the
    // pixel.  Since templating happens at compile time we can't just ask the
    // array at runtime what type to put in here (hence the "assert" above):
    //
    Image::RGBAPix<uint8_t> pixel;

    // Loop through all the pixels.  Note that the columns go from left to
    // right and the rows go from bottom to top.  (So the "row" and "column"
    // are like X and Y coordinates on the Cartesian plane, with the entire
    // image living in the quadrant with positive X and positive Y --- in other
    // words the origin is in the lower left of the image.)
    //
    cout << "Display ALL the pixels !!! " << endl;

    for (size_t row = 0; row < array.height(); ++row) {
      for (size_t col = 0; col < array.width(); ++col) {

        // read the pixel at (row, col) and print
        //
        //array.read(&pixel, row, col);
        array.read(&pixel, col, row);
        cout << "image[" << row << "," << col << "]=" <<
        (int)pixel.r << "," << (int)pixel.g << "," << (int)pixel.b << endl;
      }
    }
  }

  void onDraw(Graphics& g) {
    g.clear(0.2);

    g.pushMatrix();

      // Push the texture/quad back 5 units (away from the camera)
      //
      g.translate(0, 0, -5);

      g.quad(texture, -1, -1, 2, 2);

    g.popMatrix();
  }
};

int main() {
  MyApp app;
  app.dimensions(600, 400);
  app.title("imageTexture");
  app.start();
}
