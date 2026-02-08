//Like, Subscribe our channel: KARTIS
// https://www.youtube.com/@kartis_


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#define HISTORY 60

bool beaconEnabled = true;   // default ON

uint32_t beaconSeq = 0;
uint32_t missedPackets = 0;
uint32_t lastRemoteSeq = 0;


int rssiHist[HISTORY];
int snrHist[HISTORY];
int histIndex = 0;

String msgLog = "";
String deviceTime="--:--";

unsigned long beaconInterval=2300;
unsigned long lastBeacon = 0;
long sentPackets = 0;


#define DEST_ADDR 1

ESP8266WebServer server(80);
SoftwareSerial LoRa(D5, D6);   // RX, TX

String lastMsg="---";
int rssi=0;
int snr=0;
long packets=0;

const char* ssid="LoRa-Test-B";
const char* pass="12345678";

void sendAT(String c){
  LoRa.print(c+"\r\n");
  delay(300);
}

void handleBeacon(){
  if(server.hasArg("on")){
    beaconEnabled = server.arg("on") == "1";
  }
  server.send(200,"text/plain", beaconEnabled ? "ON" : "OFF");
}


void handleStats(){

 String json="{";
 json+="\"msg\":\""+lastMsg+"\",";
 json+="\"rssi\":"+String(rssi)+",";
 json+="\"snr\":"+String(snr)+",";

 json+="\"pkt\":"+String(packets)+",";
 json+="\"beacon\":"+String(beaconEnabled?1:0)+",";


 json+="\"log\":\""+msgLog+"\",";

 // RSSI history
 json+="\"rh\":[";
 for(int i=0;i<HISTORY;i++){
   json+=String(rssiHist[i]);
   if(i<HISTORY-1) json+=",";
 }
 json+="],";

 // SNR history
 json+="\"sh\":[";
 for(int i=0;i<HISTORY;i++){
   json+=String(snrHist[i]);
   if(i<HISTORY-1) json+=",";
 }
 json+="]";

 json+="}";

 server.send(200,"application/json",json);
}



void handleTime(){
 String t=server.arg("t");
 deviceTime=t;
 server.send(200,"ok","ok");
}


void handleSend(){
 String msg = server.arg("m");
 String cmd="AT+SEND="+String(DEST_ADDR)+","+msg.length()+","+msg;
 LoRa.print(cmd+"\r\n");

 msgLog += "<div class='tx'>["+deviceTime+"] Naan: "+msg+"</div>";

 server.send(200,"text/plain","OK");
}



void handleRoot(){

String page=R"rawliteral(
<!DOCTYPE html>
<html>
<meta name=viewport content="width=device-width">
<style>
body{background:#0b0f14;color:#e6e6e6;font-family:system-ui;margin:0}
.card{background:#111827;border-radius:12px;padding:10px;margin:10px}
canvas{width:100%;height:200px}
#log{max-height:250px;overflow:auto}
.rx{background:#1f2937;margin:5px;padding:6px;border-radius:8px}
.tx{background:#065f46;margin:5px;padding:6px;border-radius:8px;text-align:right}
input,button{width:100%;padding:10px;margin-top:10px;border-radius:8px;border:none}
button{background:#00ffd5}
.header{
  background:linear-gradient(135deg,#020617,#020617,#022c22);
  padding:15px;
  margin:10px;
  border-radius:14px;
  box-shadow:0 0 20px rgba(0,255,200,.2);
}

.title{
  font-size:22px;
  letter-spacing:2px;
  color:#00ffd5;
  font-weight:600;
}

.status{
  margin-top:6px;
  display:flex;
  align-items:center;
  gap:8px;
  color:#6ee7b7;
}

#beaconDot{
  width:10px;
  height:10px;
  border-radius:50%;
  background:#00ff88;
  box-shadow:0 0 10px #00ff88;
}

#beaconBtn{
  background:#00ffd5;
  color:black;
  font-weight:600;
}

</style>
<div class=header>
  <div class=title>LORA on ESP8266</div>
  <div class=status>
    <span id=beaconDot></span>
    <span id=beaconText>BEACON ON</span>
  </div>
</div>

<div class=card>
  <button id=beaconBtn onclick="toggleBeacon()">TURN BEACON OFF</button>
</div>


<div class=card><canvas id=g></canvas></div>

<div class=card>
RSSI <span id=rssi></span> dBm |
SNR <span id=snr></span> dB |
Packets <span id=pkt></span> |<br>



<div class=card>
<input id=m placeholder="Message">
<button onclick=send()>SEND</button>
</div>

<div class=card id=log></div>
<div>
  <footer style="text-align:center;margin-top:2px;"> Subscribe <a href="https://www.youtube.com/@kartis_">KARTIS</a> 💖</footer>
</div>

<script>

const c=document.getElementById("g");
const ctx=c.getContext("2d");
let beaconState = true;


function toggleBeacon(){
 fetch("/beacon?on="+(beaconState?0:1));
}


// make canvas real resolution (important!)
function resize(){
 c.width=c.offsetWidth;
 c.height=200;
}
resize();
window.onresize=resize;


// moving average (smooth)
function smooth(arr, w=5){
 let out=[];
 for(let i=0;i<arr.length;i++){
   let s=0,n=0;
   for(let j=-w;j<=w;j++){
     if(arr[i+j]!=undefined){
       s+=arr[i+j]; n++;
     }
   }
   out.push(s/n);
 }
 return out;
}

// map RSSI (-120..-30) → canvas Y
function mapRSSI(v){
 const topPad = 20;
 const botPad = 20;
 const usable = c.height - topPad - botPad;

 // RSSI range: -120 .. -30  (span = 90)
 return c.height - botPad - ((v + 120) * usable / 90);
}

function reorder(buf, idx){
 let out=[];
 for(let i=0;i<buf.length;i++){
   out.push(buf[(idx+i)%buf.length]);
 }
 return out;
}

function draw(r){

 ctx.clearRect(0,0,c.width,c.height);

 // grid
 ctx.strokeStyle="#222";
 for(let i=0;i<5;i++){
  ctx.beginPath();
  ctx.moveTo(0,i*c.height/5);
  ctx.lineTo(c.width,i*c.height/5);
  ctx.stroke();
 }
 // Y-axis labels
ctx.fillStyle="#6ee7b7";
ctx.font="12px system-ui";

ctx.fillText("-30 dBm", 5, mapRSSI(-30) + 4);
ctx.fillText("-60 dBm", 5, mapRSSI(-60) + 4);
ctx.fillText("-90 dBm", 5, mapRSSI(-90) + 4);
ctx.fillText("-120 dBm",5, mapRSSI(-120) + 4);


 // smooth RSSI
 r = r.map(v => v==0 ? -120 : v);
let sr = smooth(r,3);


 // RSSI line (green)
 ctx.strokeStyle="#00ff88";
 ctx.beginPath();
 sr.forEach((v,i)=>{
   let x = i*c.width/r.length;
   let y = mapRSSI(v);
   ctx.lineTo(x,y);
 });
 ctx.stroke();

 // labels
 ctx.fillStyle="#00ff88";
 ctx.fillText("RSSI vs Time",10,12);
 ctx.fillText("-30 dBm",5,25);
 ctx.fillText("-120 dBm",5,c.height-5);
}

function send(){
 fetch("/send?m="+encodeURIComponent(m.value));
 m.value="";
}

function update(){
 fetch("/stats").then(r=>r.json()).then(d=>{
  beaconState = d.beacon==1;

  rssi.innerHTML=d.rssi;
  snr.innerHTML=d.snr;
  pkt.innerHTML=d.pkt;

beaconText.innerHTML = beaconState ? "BEACON ON" : "BEACON OFF";
beaconBtn.innerHTML = beaconState ? "TURN BEACON OFF" : "TURN BEACON ON";

beaconDot.style.background = beaconState ? "#00ff88" : "#ff0033";
beaconDot.style.boxShadow = beaconState ? "0 0 10px #00ff88" : "0 0 10px #ff0033";



  log.innerHTML=d.log;
  log.scrollTop=log.scrollHeight;

let idx = d.rh.findIndex(v=>v!=-120);
if(idx<0) idx=0;

let ordered = reorder(d.rh, idx);
draw(ordered);

 });
}

// push phone time
setInterval(()=>{
 let d=new Date();
 fetch("/time?t="+d.getHours()+":"+d.getMinutes()+":"+d.getSeconds());
},5000);

// refresh stats + graph
setInterval(update,2000);
</script>







</html>
)rawliteral";

server.send(200,"text/html",page);
}



void setup(){

 Serial.begin(115200);
 for(int i=0;i<HISTORY;i++){
  rssiHist[i]=-120;
  snrHist[i]=0;
}

 LoRa.begin(115200);

 delay(1500);

 sendAT("AT");
 sendAT("AT+BAND=865000000");
 sendAT("AT+NETWORKID=6");
 sendAT("AT+PARAMETER=12,4,1,7");
 sendAT("AT+ADDRESS=2");

 WiFi.softAP(ssid,pass);

 server.on("/",handleRoot);
 server.on("/send",handleSend);
 server.on("/stats",handleStats);
 server.on("/time",handleTime);
 server.on("/beacon", handleBeacon);



 server.begin();

 Serial.println("ESP8266 READY");
}
String rxLine="";

void loop(){
if(beaconEnabled && millis()-lastBeacon>beaconInterval){
String b = "*";
LoRa.print("AT+SEND="+String(DEST_ADDR)+",1,*\r\n");
sentPackets++;

  lastBeacon=millis();
}


 server.handleClient();

 while(LoRa.available()){
   char c = LoRa.read();

   if(c=='\n'){
     processLine(rxLine);
     rxLine="";
   }else{
     rxLine+=c;
   }
 }
}
void processLine(String line){

if(line.startsWith("+RCV")){
  packets++;

  int p1=line.indexOf(',');
  int p2=line.indexOf(',',p1+1);
  int p3=line.indexOf(',',p2+1);
  int p4=line.indexOf(',',p3+1);

  lastMsg = line.substring(p2+1,p3);
  rssi = line.substring(p3+1,p4).toInt();
  snr = line.substring(p4+1).toInt();

  rssiHist[histIndex] = rssi;
  snrHist[histIndex] = snr;
  histIndex = (histIndex + 1) % HISTORY;


if(!lastMsg.startsWith("*")){
  msgLog += "<div class='rx'>["+deviceTime+"] Antha Paiyan: "+lastMsg+"</div>";
}



}


}



