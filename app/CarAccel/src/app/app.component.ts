/// <reference types="cordova-plugin-ble-central" />
import { Component } from '@angular/core';
import { IonApp, IonRouterOutlet } from '@ionic/angular/standalone';



@Component({
  selector: 'app-root',
  templateUrl: 'app.component.html',
  imports: [IonApp, IonRouterOutlet],
})
export class AppComponent {

  // See BLE heart rate service http://goo.gl/wKH3X7
  private heartRate = {
    service: '180d',
    measurement: '2a37'
  };

  constructor() { }

  public scan(onScan1: any, onError: any) {
    console.log("Starting BLE scan for heart rate monitor");
    this.status("Scanning for Heart Rate Monitor");

    var foundHeartRateMonitor = false;

    const onScan = (peripheral: any) => {
      // this is demo code, assume there is only one heart rate monitor
      console.log("Found " + JSON.stringify(peripheral));
      foundHeartRateMonitor = true;

      //ble.connect(peripheral.id, this.onConnect, this.onDisconnect);
    }

    const scanFailure = (reason: any) => {
      console.log("BLE Scan Failed: " + reason);
    }

    ble.scan([], 25, onScan1, scanFailure);

    setTimeout(() => {
      if (!foundHeartRateMonitor) {
        this.status("Did not find a heart rate monitor.");
      }
    }, 5000);
  }

  private onConnect(peripheral: any) {
    this.status("Connected to " + peripheral.id);
    ble.startNotification(peripheral.id, this.heartRate.service, this.heartRate.measurement, this.onData, this.onError);
  }
  private onDisconnect(reason: any) {
    console.log("Disconnected " + reason);
    //beatsPerMinute.innerHTML = "...";
    this.status("Disconnected");
  }
  private onData(buffer: any) {
    // assuming heart rate measurement is Uint8 format, real code should check the flags
    // See the characteristic specs http://goo.gl/N7S5ZS
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
