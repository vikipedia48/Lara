# Lara
Application for converting geographic files to PNG images.
Supported formats are currently: GeoTiff, GeoJson and CSV.

## GeoTiff
The user can convert a GeoTiff image to a regular PNG image. This is intended only for those images that contain numeric values instead of RGB pixels and are therefore unreadable by image editors.

![image](https://github.com/vikipedia48/Lara/assets/37978310/75312394-2abd-4fba-9659-4c703487cc19)

### Methods of conversion

Grayscale16_MinToMax - all values are mapped to grayscale values (0 - 65535), in which the minimum value becomes 0, the maximum becomes 65535, and all others are mapped to values between the minimum and the maximum; the output is a 16 bit grayscale image
![image](https://user-images.githubusercontent.com/37978310/224423081-1445816b-96ea-4276-89cc-414c850121f3.png)

Grayscale16_Lua - all values are mapped to grayscale values (0 - 65535) by way of a Lua script.

RGB_Lua - all values are mapped to RGBA colors by way of a Lua script.

### Optional settings

The resulting image can be cropped by inputting the points of the wanted upper left pixel and bottom right pixel

The resulting image can also be split into tiles (different files), whose size can be determined either by fixed size or by the fixed number of tiles

The image can be upscaled or downscaled. 
A downscaled image will be N^2 times smaller (N is an inputted prime number) and will produce colors based on the average value of N^2 pixels. 
An upscaled image will be N^2 times larger (N is an inputted prime number). It will simply duplicate pixels. 
Image scaling is exclusive with tiling. 

## CSV
The user can convert a CSV file that contains geo-coordinates to a PNG. The file must, at the least, contain two columns which represent x and y components. The user must specify the which columns represent x and y in the file. The program will then read the points defined by x and y and place them on the image.

![image](https://github.com/vikipedia48/Lara/assets/37978310/db12aee8-e7c8-460d-b827-9b1dc2df8b3d)

### Styles
Each CSV point can have a custom style with which it will be drawn on the image. The style is defined by shape type, shape size, shape color and center color. Configuration of styles is done via a Lua script.

### Boundary coordinates
The user can manually set the x and y coordinates which serve as boundaries for the image. This can be used to crop or resize the image as all points are placed relative to their difference from boundaries.  

## GeoJson
The user can convert a GeoJson file to a PNG. 

![image](https://github.com/vikipedia48/Lara/assets/37978310/d91d22ef-a7fe-4de1-8ad7-ded0e1794b4f)

### Colors
Each GeoJson feature can have a custom color. Configuration of styles is done via a Lua script.

### Boundary coordinates
The user can manually set the x and y coordinates which serve as boundaries for the image. This can be used to crop or resize the image as all shapes are placed relative to their difference from boundaries.  

## GeoPackage
The user can convert a GeoPackage file to a PNG. Only vector shapes (FEATURES) are supported.

![image](https://github.com/vikipedia48/Lara/assets/37978310/3561b2eb-80e0-4134-a087-e532882296dc)

### Colors
Each GeoPackage feature can have a custom color. Configuration of colors is done via a Lua script.

### Boundary coordinates
The user can manually set the x and y coordinates which serve as boundaries for the image. This can be used to crop or resize the image as all shapes are placed relative to their difference from boundaries.  

## Legacy GeoTiff output modes

Grayscale16_TrueValue - values contained within Tiff rasters are directly translated to a grayscale value (0 - 65535); user can specify an offset to be applied to all values; the output is a 16 bit grayscale image
![truevaluetest0screenshot](https://user-images.githubusercontent.com/37978310/224422668-e968f566-b853-4c41-8909-46d95aaa1b90.png)

RGB_UserValues - all values are directly converted to RGBA colors the user specifies; during the configuration of this mode, all distinct Tiff values are loaded, to which the user must assign a color; the output is a RNGA image
![image](https://user-images.githubusercontent.com/37978310/224427228-a8e9426d-b33e-45d5-a34f-650b73995641.png)

RGB_UserRanges - all values are converted to RGBA colors the user specifies; during configuration of this mode, the user can specify their own values and its adjoining colors; during conversion, Tiff values will be mapped to a user value and assigned the specified color; an option is available to use gradient colors, which will not directly assign the specified colors, but instead use a color between the upper and the lower range; the output is a RNGA image
![image](https://user-images.githubusercontent.com/37978310/224425006-ea35e0ab-288c-49ed-ba79-29909275b96e.png)
![image](https://user-images.githubusercontent.com/37978310/224425385-359c04ee-de05-4193-896e-f1c448295d95.png)

RGB_Formula - all values are converted to a RGBA color determined by a formula, which is : (R -> value div 65536, G -> (value - value div 65536) div 256, B -> value mod 256); if the value is negative, Alpha value of the pixel will be 128, otherwise 255; for example, 255 -> (0,0,255,255), 256 -> (0,1,0,255), 257 -> (0,1,1,255), 65535 -> (0,255,255,255), 65536 -> (1,0,0,255), -65537 -> (1,0,1,128); the output is a RNGA image
![image](https://user-images.githubusercontent.com/37978310/224425658-f80ba917-6fa6-4c1f-81d2-c4f60af43aa7.png)

## Legacy CSV configuration

A table will be loaded with 4 style-defining columns alongside all of the CSV columns. In this image, the PNG will be created in this way:
If a CSV row has 1000 set in the column „code“, it will have a style defined in the first 4 rows of the second row. That is, a circle with the size 1, with the shape color of 255,0,0,255 (red) and the center color 255,0,255 (purple). To have size 1 means that only a single pixel will be marked, to have size larger than 1 means that the radius of the shape will be !SHAPE SIZE! - 1.
If a CSV row has *instead* Test123 set in the column „name“, it will have a style defined in the third row. If a CSV row has *instead* 2000 set in the column „population“, it will have a style defined in the fourth row. If the CSV row doesn't match any of the defined conditions or if a mistake has occured during input, it will have a style defined in the first row. Inputting the first row's style is mandatory.
The !SHAPE TYPE! table column accepts only four hard-coded strings: Square, EmptySquare, Circle and EmptyCircle.

## Legacy GeoJson configuration

A table will be loaded with 2 custom columns alongside all of the properties (of the file's first feature) that serve to provide info about the features in the file. In this image, the PNG will be created in this way:
If a GeoJson feature has 1000 set for the key „code“, it will have the color defined in the second row. If a GeoJson feature has *instead* MultiLineString as its geometry type, it will have the color defined in the third row. If a GeoJson feature doesn't match any of the defined conditions or if a mistake has occured during input, it will have the color defined in the first row. Inputting the first row's color is mandatory.
The !GEOMETRY TYPE! table column accepts only six hard-coded strings: Point, MultiPoint, LineString, MultiString, Polygon and MultiPolygon.

## Legacy GeoPackage configuration

A table will be loaded for each layer the GeoPackage contains, as well as a !ALL LAYERS! table. Each layer's table will contain custom columns which represent its attributes that serve to provide info about the features in the file. In the above images, the PNG will be created in this way:
All features of the layer 'fclass_river' will be colored like this. If the feature has "customClass" in the "fclass" column, it will have the color defined in the second row. If a feature has *instead* 1000 set in the "code" column, it will have the color defined in the third row. If a feature doesn't match any of the defined conditions or if a mistake has occured during input, it will have the color defined in the first row. Inputting the first row's color is mandatory if you wish to configure any other colors.
All other layers, which haven't been configured, will be colored like this. If whatever layer is called "LayerWhichIHaventConfigured", it will have the color defined in the second row. If a layer has *instead* MultiLineString as its specified geometry type, it will have the color defined in the third row. If a feature doesn't match any of the defined conditions or if a mistake has occured during input, it will have the color defined in the first row. Inputting the first row's color is mandatory even in the case you have configured all possible individual layers. 


# Known bugs
In geographic vector files, the y coordinates will (in most coordinate reference systems) likely go from south to north. This is true for the great majority of CRS's. As such, the application will create an image that is flipped by the Y axis. My solution is that all such images are by default flipped by the Y axis. This is a solution for the great majority of images. However, if the file's Y coordinates do indeed go from north to south, the image will be then in turn be flipped the wrong way. If this happens, you can fix the issue easily by flipping the image yourself in any image editing software.  
