# Senior Design Demining Project
A senior project for autonomous demining. 

## TODO
1) Read from the GPS properly.  
2) How do I write to the horn pin?  
```c
byte horn_pin = 10;
// ...
pinMode(horn_pin, OUTPUT);
//...
digitalWrite(horn_pin, HIGH);
```
3) How do I read from the metal detector? It doesn't seem to be a pin so maybe it's a serial port.  
4) Why does writing to Serial sometimes screw everything up?  

## Contributing
* Clone the repo
```bash
cd <some directory>
git clone https://github.com/notgate/deminer
cd deminer
git branch <your branch name>
git checkout <your branch name>
```
* Committing changes
```bash
git add . # or add the specific file you changed
git commit -m "Talk about the change you made"
git push origin <your branch>
```
