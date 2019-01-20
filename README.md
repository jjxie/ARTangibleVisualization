# Augmented physical visualization of home food consumption and nutrition
Exploring a new tangible interface and interactions in the context of helping people gain knowledge of their food consumption and nutrients intake. 
Check here for detailed information: https://jjxie8.wixsite.com/thinkbig/master-thesis-project.


# The settings of the project
This Project runs Socket IO client sides on two ESP32 boards and server side on the index.js.

The sensors and materials used:
1. Three ESP32 boards. 
2. Five load cells and five HX711 amplifiers to measure food weight.
3. Ten linear servos (SG90) + ten 3D models. Five sets represent food weight. Another five sets represent the amount of nutrients. The height of a rack of a 3D model represent a food weight data or the amount of a nutreint. 
The 3D model files from https://github.com/tscha70/3DPrinterSTLFiles/tree/master/win-D/STL-Parts with minor modifications.
4. Five touch sensors are used to do selection of food to show detailed AR information. (using AR.js) 
5. Five distance sensors (VL6180) which enable users to interact with the food weight data and see the simulated food consumption and nutrition intakes 


--
A video of the tangible interface prototype on Youtube: https://www.youtube.com/watch?v=QyQpSfhoo6U
A video of comparing screen interface prototype on Youtube:https://www.youtube.com/watch?v=5_SD-PaekHA
