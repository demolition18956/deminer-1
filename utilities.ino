void PrintLogo() {   
    runTime =  millis()/1000.0; 
    Serial1.print(runTime, 3); 
    Serial1.write(9); 
    Serial1.println(" "); 
}

void PrintMon() {     
    runTime =  millis()/1000.0; 
    Serial.print(runTime, 3); 
    Serial.println(" ");      
}
