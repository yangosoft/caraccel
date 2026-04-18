/// <reference types="cordova-plugin-ble-central" />
import { Component } from '@angular/core';
import { CommonModule } from '@angular/common';
import { IonHeader, IonToolbar, IonTitle, IonContent, IonButton, IonList, IonLabel, IonItem, IonCard, IonGrid, IonRow, IonCol, IonCardContent, IonCardTitle, IonCardSubtitle, IonCardHeader } from '@ionic/angular/standalone';
import { ExploreContainerComponent } from '../explore-container/explore-container.component';
import { App } from '@capacitor/app';
import { AppComponent } from '../app.component';



interface Historic {
  from_0_to_80_ms: number;
  from_80_to_100_ms: number;
  from_100_to_120_ms: number;
};



@Component({
  selector: 'app-tab1',
  templateUrl: 'tab1.page.html',
  styleUrls: ['tab1.page.scss'],
  imports: [CommonModule, IonHeader, IonCardHeader, IonCardContent, IonToolbar, IonTitle, IonContent, IonButton, IonList, IonLabel, IonItem, IonCard, IonGrid, IonRow, IonCol, IonCardContent, IonCardTitle, IonCardSubtitle, IonCardHeader],
})
export class Tab1Page {

  public static readonly MAX_NUMBER_OF_ENTRIES = 128;
  public devices: BLECentralPlugin.PeripheralData[] = [];
  public historic: Historic[] = new Array(Tab1Page.MAX_NUMBER_OF_ENTRIES);
  public peripheral: any;
  public isGettingData: boolean = false;
  public statusMessage: string = "Disconnected";
  constructor() {
    for (let i = 0; i < Tab1Page.MAX_NUMBER_OF_ENTRIES; i++) {
      this.historic[i] = {
        from_0_to_80_ms: 0,
        from_80_to_100_ms: 0,
        from_100_to_120_ms: 0
      }
    }

  }

  public exportToTXT() {

    //Create a text file with the historic data and download it
    let text = "from_0_to_80_ms,from_80_to_100_ms,from_100_to_120_ms\n";
    this.historic.forEach((entry) => {
      text += entry.from_0_to_80_ms + "," + entry.from_80_to_100_ms + "," + entry.from_100_to_120_ms + "\n";
    });

    const blob = new Blob([text], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    //use day and time in the file name
    a.download = 'historic_data_' + new Date().toISOString() + '.txt';
    a.click();
    URL.revokeObjectURL(url);

  }

  public getData() {

    if (this.isGettingData) {
      console.log("Already getting data, please wait...");
      this.statusMessage = "Already getting data, please wait...";
      return;
    }

    this.isGettingData = true;
    for (let i = 0; i < Tab1Page.MAX_NUMBER_OF_ENTRIES; i++) {


      //wait 120 ms between each read to avoid overwhelming the device
      setTimeout(() => {
        this.statusMessage = "Getting data... (" + (i + 1) + "/" + Tab1Page.MAX_NUMBER_OF_ENTRIES + ")";
        this.readData();
      }, 300 * i);
    }



  }

  private readData() {
    if (this.peripheral) {

      // Implement the logic to get data from the connected device
      ble.read(this.peripheral.id, "0100bc9a-7856-3412-f0de-bc9a78563412", "0200bc9a-7856-3412-f0de-bc9a78563412", (adata: ArrayBuffer) => {




        // This is an array of 8 uint32_t
        // [0] is the type, 3 for historic values
        // [1] for historic values, indicates the index
        // [2..7] are times in milliseconds, little endian

        var data: Uint16Array = new Uint16Array(adata);
        if (data.length != 8) {
          console.log("Unexpected data length: " + data.length);
          return;
        }

        if (data[0] != 3) {
          console.log("Unexpected data type: " + data[0]);
          return;
        }

        console.log("Received data in hex: " + Array.from(new Uint8Array(adata)).map(b => b.toString(16).padStart(2, '0')).join(' '));


        var index = data[1];


        this.historic[index].from_0_to_80_ms = data[2];
        this.historic[index].from_80_to_100_ms = data[3];
        this.historic[index].from_100_to_120_ms = data[4];



        this.historic[index + 1].from_0_to_80_ms = data[5];
        this.historic[index + 1].from_80_to_100_ms = data[6];
        this.historic[index + 1].from_100_to_120_ms = data[7];


        if (index == Tab1Page.MAX_NUMBER_OF_ENTRIES - 2) {
          this.statusMessage = "Completed getting data";
          this.isGettingData = false;
        }

      }, (error: any) => {
        console.log("Read failed: " + error);
      });
    }


  }

  public disconnect() {
    if (this.peripheral) {
      ble.disconnect(this.peripheral.id, () => {
        console.log("Disconnected from " + this.peripheral.id);
        this.peripheral = null;
        this.statusMessage = "Disconnected";
      }, (error: any) => {
        console.log("Disconnect failed: " + error);
      });
    }
  }

  public scan() {

    this.statusMessage = "Scanning for devices...";
    AppComponent.prototype.scan((peripheral: BLECentralPlugin.PeripheralData) => {
      console.log("Scan successful: " + JSON.stringify(peripheral));
      //insert in list from tab1.page.html
      console.log(" |-> Found " + peripheral.id + " with name " + peripheral.name);
      //this.statusMessage = "Found " + peripheral.name + " (" + peripheral.id + ")";
      this.devices.push(peripheral);

      if (peripheral.name === "CARACCEL") {
        ble.stopScan();
        console.log(" |-> Found CARACCEL device");
        this.statusMessage = "Connecting to " + peripheral.name + " (" + peripheral.id + ")...";
        ble.connect(peripheral.id, this.onConnect, this.onDisconnect);
        this.peripheral = peripheral;

      }

    }, (error: any) => {
      console.log("Scan failed: " + error);
      this.statusMessage = "Scan failed: " + error;
    });
  }

  private onConnect(peripheral: any) {
    this.status("Connected to " + peripheral.id);
    this.statusMessage = "Connected to " + peripheral.id;
    //ble.startNotification(peripheral.id, this.heartRate.service, this.heartRate.measurement, this.onData, this.onError);

  }
  private onDisconnect(reason: any) {
    console.log("Disconnected " + reason);
    //beatsPerMinute.innerHTML = "...";
    this.status("Disconnected");
    this.peripheral = null;
  }
  private onData(buffer: any) {
    var data = new Uint8Array(buffer);
    //beatsPerMinute.innerHTML = data[1];
    console.log("Received heart rate measurement: " + data[1]);
  }
  private onError(reason: any) {
    console.log("There was an error " + reason);
  }
  private status(message: any) {
    console.log(message);
    //statusDiv.innerHTML = message;
  }
}
