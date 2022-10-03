# pfodGUIdesigner
The pfodGUIdesigner.ino runs on an ESP32-C3 and provides drawing package that lets you design your own custom Arduino Graphical User Interface (GUI) components
for Android/pfodApp and generate the Arduino class code that implements them.  

The final designed components are very light weight and can easily run on an UNO connected to a communication shield.
The general purpose [pfodApp](https://www.forward.com.au/pfod/index.html) Android app is used as the display for both the designer and for the final design.
The final GUI design can connect via either WiFi or Bluetooth or BLE or SMS and the free [pfodDesignerV3](https://www.forward.com.au/pfod/pfodDesigner/index.html)
Android app will generated the Arduino code for these connections for a wide variety of Arduino and compatible boards.

The generated code can be saved to a file on your mobile, but is also dumped to the Arduino monitor so you can copy and paste from there.  

See the [Arduino GUI Designer and Code Generator](https://www.forward.com.au/pfod/ESPAutoWiFiConfig/index.html) tutorial for detailed examples and videos.
The GUI designer uses a set of low level elements that can be combined to create sophisticated GUIs.
The basic dwg elements are rectangles, lines, circles, arcs, labels, touchZones and touchActions/Inputs (dialog text edit box).
Not all the options available for these elements are covered by the designer.
See the [pfodSpecification.pdf](https://www.forward.com.au/pfod/pfodSpecification.pdf) for all the details and the other supporting commands like pushZero()/popZero(), hide()/unhide() and insertDwg().
The pfodGUIdesigner itself uses these elements to create its own interface, so check out the pfodGUIdesigner library files for examples of advanced usage.

This library needs the [SafeString](https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html) and [pfodParser](https://www.forward.com.au/pfod/pfodParserLibraries/index.html)
libraries to be installed as well.

# How-To
To start the GUI designer, open the pfodGUIdesiger example sketch, set your network ssid/password, compile and load and connect to pfodApp to show the GUI designer interface.    
See [Arduino GUI Designer and Code Generator](https://www.forward.com.au/pfod/ESPAutoWiFiConfig/index.html)  

# Software License
(c)2022 Forward Computing and Control Pty. Ltd.  
NSW Australia, www.forward.com.au  
This code is not warranted to be fit for any purpose. You may only use it at your own risk.  
This code may be freely used for both private and commercial use  
Provide this copyright is maintained.    

# Revisions
Version 1.0.46 first release     
   
