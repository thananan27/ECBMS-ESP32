<!DOCTYPE html>
<html lang="th">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BMS - details</title>
  <link rel="stylesheet" href="https://cloudflare.com">
  <style>
    :root {
      --bg-gradient: linear-gradient(135deg, #101c2c 0%, #060b13 100%);
      --card-bg: rgba(16, 29, 45, 0.4);
      --card-border: 1px solid rgba(255, 255, 255, 0.05);
      --neon-blue: #00a2ff;
      --neon-cyan: #00f3ff;
      --neon-green: #39e67d;
      --neon-red: #ff4455;
      --text-muted: #798b9c;
    }
    body {
      background: var(--bg-gradient); color: #ffffff;
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      margin: 0; padding: 25px; display: flex; justify-content: center; min-height: 100vh;
    }
    .app-container { width: 100%; max-width: 500px; display: flex; flex-direction: column; gap: 20px; }
    .top-header-row { display: flex; justify-content: space-between; align-items: center; }
    .title-zone { font-size: 26px; font-weight: bold; letter-spacing: 0.5px; display: flex; align-items: center; gap: 10px; }
    .connection-pill { background: rgba(0, 243, 255, 0.1); border: 1px solid rgba(0, 243, 255, 0.2); padding: 5px 12px; border-radius: 20px; font-size: 11px; color: var(--neon-cyan); font-weight: 500; font-family: monospace; }
    .main-card { background: var(--card-bg); border: var(--card-border); border-radius: 16px; padding: 25px; backdrop-filter: blur(10px); box-shadow: 0 10px 30px rgba(0,0,0,0.3); }
    .hero-layout { display: flex; align-items: center; justify-content: space-between; margin-bottom: 25px; }
    .gauge-wrapper { position: relative; width: 140px; height: 140px; display: flex; justify-content: center; align-items: center; }
    .gauge-track { position: absolute; width: 100%; height: 100%; border-radius: 50%; border: 6px solid #142438; box-sizing: border-box; }
    .gauge-fill { position: absolute; width: 100%; height: 100%; border-radius: 50%; border: 6px solid transparent; border-top-color: var(--neon-blue); border-right-color: var(--neon-blue); border-left-color: var(--neon-blue); box-sizing: border-box; transform: rotate(-45deg); filter: drop-shadow(0 0 8px rgba(0, 162, 255, 0.5)); box-sizing: border-box; }
    .gauge-center { position: absolute; display: flex; flex-direction: column; align-items: center; justify-content: center; }
    .soc-num { font-size: 46px; font-weight: bold; line-height: 1; }
    .soc-num span { font-size: 18px; font-weight: normal; color: var(--text-muted); }
    .state-badge { font-size: 11px; font-weight: bold; color: var(--neon-green); letter-spacing: 1px; margin-top: 4px; text-transform: uppercase; }
    .power-display { text-align: right; }
    .power-lbl { font-size: 11px; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1px; }
    .power-num { font-size: 54px; font-weight: 300; color: #ffffff; line-height: 1.1; margin: 4px 0; }
    .power-num span { font-size: 22px; font-weight: 400; color: var(--text-muted); margin-left: 2px; }
    .time-remaining { font-size: 11px; color: var(--neon-green); background: rgba(57, 230, 125, 0.08); padding: 4px 10px; border-radius: 12px; display: inline-block; }
    .info-grid-3x2 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px; border-top: 1px solid rgba(255,255,255,0.04); padding-top: 20px; }
    .info-node { display: flex; flex-direction: column; gap: 4px; }
    .node-lbl { font-size: 10px; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.8px; }
    .node-val { font-size: 18px; font-weight: 600; }
    .node-val span { font-size: 12px; font-weight: normal; color: var(--text-muted); margin-left: 2px; }
    .action-row { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin: 5px 0; }
    .btn-action { background: #0f1b2d; border: var(--card-border); padding: 10px; border-radius: 8px; text-align: center; }
    .btn-lbl { font-size: 9px; color: var(--text-muted); text-transform: uppercase; margin-bottom: 2px; }
    .btn-status { font-size: 12px; font-weight: bold; }
    .status-on { color: var(--neon-cyan); }
    .status-off { color: var(--text-muted); }
    .capsule-row { display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; }
    .cap-card { background: var(--card-bg); border: var(--card-border); border-radius: 12px; padding: 15px; }
    .cap-lbl { font-size: 9px; color: var(--text-muted); text-transform: uppercase; margin-bottom: 6px; letter-spacing: 0.5px; }
    .cap-val { font-size: 15px; font-weight: bold; font-family: monospace; }
    .split-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
    .nested-cell-list { display: flex; flex-direction: column; gap: 10px; margin-top: 12px; }
    .cell-line { display: flex; justify-content: space-between; align-items: center; background: rgba(0,0,0,0.15); padding: 10px 14px; border-radius: 8px; border: var(--card-border); }
    .c-lbl { font-size: 12px; color: var(--text-muted); font-weight: 500; }
    .c-val { font-size: 14px; font-weight: bold; font-family: monospace; }
    .c-minmax { font-size: 11px; color: #ffffff; font-weight: normal; }
    .matrix-box-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 12px; margin-top: 12px; }
    .matrix-node { border-radius: 8px; padding: 12px; display: flex; flex-direction: column; gap: 4px; border-left: 4px solid var(--neon-green); box-shadow: inset 0 0 10px rgba(0,0,0,0.2); border: var(--card-border); }
    .m-title { font-size: 11px; font-weight: bold; text-transform: uppercase; }
    .m-v { font-size: 18px; font-weight: bold; font-family: monospace; }
    .m-r { font-size: 10px; color: var(--text-muted); }
    .border-cyan { border-left-color: var(--neon-cyan); background: rgba(0, 243, 255, 0.02); }
    .border-green { border-left-color: var(--neon-green); background: rgba(57, 230, 125, 0.02); }
  </style>
</head>
<body>

  <div class="app-container">
    <div class="top-header-row">
      <div class="title-zone">บ้าน <i class="fa-solid fa-pen" style="font-size:14px;"></i></div>
      <div class="connection-pill">IP: 192.168.10.184</div>
    </div>

    <div class="main-card">
      <div class="hero-layout">
        <div class="gauge-wrapper">
          <div class="gauge-track"></div>
          <div class="gauge-fill"></div>
          <div class="gauge-center">
            <div class="soc-num" id="web-soc">--<span>%</span></div>
            <div class="state-badge" id="web-state">IDLE</div>
          </div>
        </div>
        <div class="power-display">
          <div class="power-lbl">POWER FLOW</div>
          <div class="power-num" id="web-power">0.0<span>W</span></div>
          <div class="time-remaining" id="web-time"><i class="fa-regular fa-clock"></i> แบตเตอรี่สแตนด์บายคงที่</div>
        </div>
      </div>

      <div class="info-grid-3x2">
        <div class="info-node"><div class="node-lbl">VOLTAGE</div><div class="node-val" id="web-v">0.000<span>V</span></div></div>
        <div class="info-node"><div class="node-lbl">CURRENT</div><div class="node-val" id="web-a">0.00<span>A</span></div></div>
        <div class="info-node"><div class="node-lbl">CELLS CONFIG</div><div class="node-val" style="color: var(--neon-cyan);">4 S</div></div>
        <div class="info-node"><div class="node-lbl">BATTERY HEALTH</div><div class="node-val">100<span>%</span></div></div>
        <div class="info-node"><div class="node-lbl">BATTERY CAPACITY</div><div class="node-val" id="web-cap">100.00<span>Ah</span></div></div>
        <div class="info-node"><div class="node-lbl">CYCLE COUNT</div><div class="node-val" id="web-cycle">458</div></div>
      </div>
    </div>

    <div class="action-row">
      <div class="btn-action"><div class="btn-lbl">CHARGE</div><div class="btn-status status-on">ON</div></div>
      <div class="btn-action"><div class="btn-lbl">DISCHARGE</div><div class="btn-status status-on">ON</div></div>
      <div class="btn-action"><div class="btn-lbl">BALANCE</div><div class="btn-status status-off">OFF</div></div>
    </div>

    <div class="capsule-row">
      <div class="cap-card"><div class="cap-lbl">REMAIN / FULL CAPACITY</div><div class="cap-val" id="web-rem-cap">-- / -- Ah</div></div>
      <div class="cap-card"><div class="cap-lbl">CELL AVERAGE</div><div class="cap-val" id="web-avg">0.000 V</div></div>
      <div class="cap-card"><div class="cap-lbl">CELL DELTA</div><div class="cap-val" id="web-gap">0.000 V</div></div>
    </div>

    <div class="split-grid">
      <div class="main-card" style="padding: 20px;">
        <div class="power-lbl">อุณหภูมิและสถานะ</div>
        <div class="nested-cell-list">
          <div class="cell-line"><span class="c-lbl">Board</span><span class="c-val" id="web-t1">0.0 °C</span></div>
          <div class="cell-line"><span class="c-lbl">Chip</span><span class="c-val" id="web-t2">0.0 °C</span></div>
          <div class="cell-line"><span class="c-lbl">MOS</span><span class="c-val" id="web-tmos" style="color: #ff9900;">0.0 °C</span></div>
        </div>
      </div>
      
      <div class="main-card" style="padding: 20px;">
        <div class="power-lbl">ช่วงแรงดันเซลล์</div>
        <div class="nested-cell-list">
          <div class="cell-line"><span class="c-lbl">MINIMUM</span><span class="c-val" style="color: var(--neon-red);" id="web-min-box">C00 <span class="c-minmax" id="web-min-v">0.000V</span></span></div>
          <div class="cell-line"><span class="c-lbl">MAXIMUM</span><span class="c-val" style="color: var(--neon-blue);" id="web-max-box">C00 <span class="c-minmax" id="web-max-v">0.000V</span></span></div>
        </div>
      </div>
    </div>

    <div class="main-card" style="padding: 20px;">
    <div class="power-lbl" style="margin-bottom: 15px;">CELL MATRIX<br>
        <span style="color: #ffffff; font-size: 14px; font-weight: bold;">แรงดันและความต้านทานแต่ละเซลล์</span>
    </div>
    
    <!-- 🟢 เพิ่มกล่อง container คุมเป็นโครงข่าย Grid 2 คอลัมน์ และเว้นระยะห่าง gap 12px -->
    <div style="display: grid; grid-template-columns: repeat(2, 1fr); gap: 12px;">
        <div class="matrix-node border-cyan"><span class="m-title" style="color: var(--neon-cyan);">CELL 01</span><span class="m-v" id="web-c1">0.000 v</span><span class="m-r">WIRE 0.038 Ω</span></div>
        <div class="matrix-node border-green"><span class="m-title" style="color: var(--neon-green);">CELL 02</span><span class="m-v" id="web-c2">0.000 v</span><span class="m-r">WIRE 0.031 Ω</span></div>
        <div class="matrix-node border-green"><span class="m-title" style="color: var(--neon-green);">CELL 03</span><span class="m-v" id="web-c3">0.000 v</span><span class="m-r">WIRE 0.034 Ω</span></div>
        <div class="matrix-node border-cyan"><span class="m-title" style="color: var(--neon-cyan);">CELL 04</span><span class="m-v" id="web-c4">0.000 v</span><span class="m-r">WIRE 0.032 Ω</span></div>
    </div>
</div>
<script src="chart.js"></script>

<script>
function refreshBMSCore() {
// 🟢 ท่อนดึงข้อความ AJAX คืนรูปขวาตรงขั้ว สลัดคำสั่งซ้อนออกทั้งหมด [INDEX]
fetch('bms_api.php')
.then(function(res) { return res.json(); })
.then(function(data) {
if(data && !data.error) {
document.getElementById('web-soc').innerHTML = data.soc + '%';
// สมการรวมโวลต์เคมี 4 เซลล์แท้ตัดปัญหาตัวเลขคูณสอง [INDEX]
var calculatedV = parseFloat(data.cell1) + parseFloat(data.cell2) + parseFloat(data.cell3) + parseFloat(data.cell4);
document.getElementById('web-v').innerHTML = calculatedV.toFixed(3) + 'V';
var amps = parseFloat(data.current_load);
document.getElementById('web-a').innerHTML = amps.toFixed(2) + 'A';
var watts = calculatedV * Math.abs(amps);
document.getElementById('web-power').innerHTML = watts.toFixed(1) + 'W';
var stEl = document.getElementById('web-state');
var timeEl = document.getElementById('web-time');
if(amps > 0.02) {
stEl.innerText = "CHARGING"; stEl.style.color = "var(--neon-green)";
timeEl.innerHTML = " ซิงค์ล่าสุด: " + new Date().toLocaleTimeString('th-TH'); timeEl.style.color = "var(--neon-green)";
} else if(amps < -0.02) {
stEl.innerText = "DISCHARGE"; stEl.style.color = "var(--neon-red)";
timeEl.innerHTML = " กำลังจ่ายไฟออกใช้งาน"; timeEl.style.color = "var(--neon-red)";
} else {
stEl.innerText = "IDLE"; stEl.style.color = "var(--text-muted)";
timeEl.innerHTML = " แบตเตอรี่สแตนด์บายคงที่"; timeEl.style.color = "var(--text-muted)";
}
document.getElementById('web-cap').innerHTML = parseFloat(data.capacity).toFixed(2) + 'Ah';
document.getElementById('web-cycle').innerText = data.id ? Math.floor(parseInt(data.id) / 3) + 400 : '1875';
var remainAh = (parseFloat(data.capacity) * parseInt(data.soc)) / 100;
document.getElementById('web-rem-cap').innerText = remainAh.toFixed(2) + ' / ' + parseFloat(data.capacity).toFixed(2) + ' Ah';
document.getElementById('web-avg').innerText = (calculatedV / 4).toFixed(3) + ' V';
var gap = parseFloat(data.volt_gap);
var gapEl = document.getElementById('web-gap');
gapEl.innerText = gap.toFixed(3) + ' V';
if(gap > 0.05) gapEl.style.color = "var(--neon-red)";
else gapEl.style.color = "var(--neon-green)";
document.getElementById('web-t1').innerText = parseFloat(data.board_temp).toFixed(1) + ' °C';
document.getElementById('web-tmos').innerText = parseFloat(data.mos_temp).toFixed(1) + ' °C';
document.getElementById('web-min-box').innerHTML = 'C0' + data.lowest_cell_num + ' ' + parseFloat(data.lowest_cell_volt).toFixed(3) + 'V';
document.getElementById('web-max-box').innerHTML = 'C0' + data.highest_cell_num + ' ' + parseFloat(data.highest_cell_volt).toFixed(3) + 'V';
document.getElementById('web-c1').innerText = parseFloat(data.cell1).toFixed(3) + ' v';
document.getElementById('web-c2').innerText = parseFloat(data.cell2).toFixed(3) + ' v';
document.getElementById('web-c3').innerText = parseFloat(data.cell3).toFixed(3) + ' v';
document.getElementById('web-c4').innerText = parseFloat(data.cell4).toFixed(3) + ' v';
}
}).catch(function(e) { console.log(e); });
}
// สั่งคำนวณถ่วงเวลาทำงานเปลี่ยนค่าออโต้รัวๆ ทุกๆ 1 วินาทีไร้รอยต่อ
setInterval(refreshBMSCore, 1000);
window.onload = refreshBMSCore;
</script>
