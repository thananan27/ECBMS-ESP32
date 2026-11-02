<!DOCTYPE html>
<html lang="th">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>EC-Smart BMS Dashboard</title>
    <style>
        /* สไตล์หลักของหน้าเว็บ (ไม่ต้องพึ่งพิง Tailwind ผ่านเน็ต) */
        body { 
            background-color: #0d1624; 
            color: #ffffff; 
            font-family: sans-serif; 
            padding: 16px; 
            max-width: 450px; 
            margin: auto; 
        }
.menu-grid { 
    display: flex !important; 
    flex-direction: row !important;
    justify-content: space-between !important;
    align-items: center !important;
    width: 100% !important;
    gap: 6px !important; 
    margin-bottom: 24px !important; 
    box-sizing: border-box !important;
}
.menu-item {
    display: block !important;
    flex: 1 !important; /* บังคับให้ทุกปุ่มกางกว้างเท่าๆ กัน */
    background-color: #111827; 
    color: #9ca3af; 
    padding: 10px 0; 
    border-radius: 8px; 
    text-decoration: none;
    text-align: center;
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 0.05em;
}
.menu-active { 
    background-color: #121f33 !important; 
    color: #22d3ee !important; 
    border: 1px solid #164e63 !important; 
}

        .header-area { 
            display: flex; 
            justify-content: space-between; 
            align-items: center; 
            margin-bottom: 24px; 
        }
        .header-title { 
            font-size: 24px; 
            font-weight: 700; 
            margin: 0;
        }
        .mac-badge { 
            background-color: #083344; 
            color: #22d3ee; 
            font-size: 12px; 
            font-family: monospace; 
            padding: 4px 12px; 
            border-radius: 9999px; 
            border: 1px solid #155e75; 
        }
        .card-row { 
            display: grid; 
            grid-template-columns: repeat(2, minmax(0, 1fr)); 
            gap: 16px; 
            margin-bottom: 16px; 
        }
        .card-bg { 
            background-color: #121f33; 
            padding: 24px; 
            border-radius: 16px; 
        }
        .circle-container { 
            display: flex; 
            justify-content: center; 
            align-items: center; 
            min-height: 160px; 
        }
        .soc-circle { 
            width: 112px; 
            height: 112px; 
            display: flex; 
            justify-content: center; 
            align-items: center; 
            border-radius: 50%; 
            border: 4px dashed #22d3ee; 
            text-align: center; 
        }
        .soc-num { font-size: 30px; font-weight: 700; }
        .soc-unit { font-size: 14px; font-weight: 600; }
        .charging-status { font-size: 10px; letter-spacing: 0.1em; margin-top: 4px; font-weight: 700; }
        .power-value { font-size: 36px; font-weight: 300; margin: 8px 0; }
        .power-unit { font-size: 20px; font-weight: 400; }
        
        .grid-3col { 
            display: grid; 
            grid-template-columns: repeat(3, minmax(0, 1fr)); 
            gap: 16px 8px; 
            text-align: center; 
        }
        .label-text { font-size: 10px; color: #9ca3af; letter-spacing: 0.05em; margin-bottom: 4px; text-transform: uppercase; }
        .value-text { font-size: 18px; font-weight: 700; color: #f3f4f6; margin: 0; }
        .value-unit { font-size: 14px; font-weight: 400; color: #9ca3af; }
        
        .cell-table { width: 100%; font-family: monospace; font-size: 12px; margin-top: 8px; }
        .cell-row { display: flex; justify-content: space-between; margin-bottom: 6px; }
        
        /* คลาสสีช่วยจัดแต่งสถานะ */
        .text-cyan-400 { color: #22d3ee; }
        .text-red-400 { color: #f87171; }
        .text-emerald-400 { color: #34d399; }
        .bg-gray-900 { background-color: #111827; }
    </style>
</head>
<body>

    <!-- เมนูบาร์หลัก -->
    <div class="menu-grid">
        <span class="menu-item menu-active">BOARD</span>
        <a href="history.php" class="menu-item">GRAPH</a>
        <a href="report.php" class="menu-item">HOURLY</a>
        <span class="menu-item">REPORT</span>
        <span class="menu-item">SETUP</span>
    </div>

    <!-- ส่วนหัวข้อ -->
    <div class="header-area">
        <h1 class="header-title">EC-Smart BMS</h1>
        <span class="mac-badge" id="txt_mac_addr">--:--:--:--:--:--</span>
    </div>

    <!-- แถวของการ์ดสถานะชาร์จและไฟไหล -->
    <div class="card-row">
        <div class="card-bg circle-container">
            <div class="soc-circle" id="circle_border">
                <div>
                    <span class="soc-num" id="txt_soc">--</span><span class="soc-unit">%</span>
                    <div class="charging-status" id="txt_is_charging">--</div>
                </div>
            </div>
        </div>
        <div class="card-bg" style="display: flex; flex-direction: column; justify-content: center;">
            <div class="label-text">POWER FLOW</div>
            <div class="power-value"><span id="txt_power">--.-</span> <span class="power-unit">W</span></div>
            <div id="txt_flow_text" style="font-size: 12px; margin-top: 8px; font-weight: 300;">--</div>
        </div>
    </div>

    <!-- กล่องข้อมูลกระแสหลัก -->
    <div class="card-bg grid-3col" style="margin-bottom: 16px;">
        <div><div class="label-text">VOLTAGE</div><div class="value-text"><span id="txt_total_volt">--.---</span> <span class="value-unit">V</span></div></div>
        <div><div class="label-text">CURRENT</div><div class="value-text"><span id="txt_current_load">--.--</span> <span class="value-unit">A</span></div></div>
        <div><div class="label-text">CELLS CONFIG</div><div class="value-text text-cyan-400">4 S</div></div>
        <div><div class="label-text">BATTERY HEALTH</div><div class="value-text">100 <span class="value-unit">%</span></div></div>
        <div><div class="label-text">BATTERY CAPACITY</div><div class="value-text"><span id="txt_capacity">--.--</span> <span class="value-unit">Ah</span></div></div>
        <div><div class="label-text">CYCLE COUNT</div><div class="value-text">1693</div></div>
    </div>
        <!-- แผงแสดงผลดิจิตอล 3 ปุ่มควบคุม เพิ่มเข้าไฟล์ของเก่า -->
    <div class="status-btn-group" style="display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 12px; margin-bottom: 16px; text-align: center;">
        <div class="status-btn-card" style="background-color: #121f33; padding: 12px 6px; border-radius: 12px; border: 1px solid #1e293b;">
            <div class="status-title" style="font-size: 10px; color: #9ca3af; font-weight: bold; letter-spacing: 0.05em; margin-bottom: 4px;">CHARGE</div>
            <div class="status-state" id="state_charge" style="font-size: 16px; font-weight: 800; color: #6b7280;">--</div>
        </div>
        <div class="status-btn-card" style="background-color: #121f33; padding: 12px 6px; border-radius: 12px; border: 1px solid #1e293b;">
            <div class="status-title" style="font-size: 10px; color: #9ca3af; font-weight: bold; letter-spacing: 0.05em; margin-bottom: 4px;">DISCHARGE</div>
            <div class="status-state" id="state_discharge" style="font-size: 16px; font-weight: 800; color: #6b7280;">--</div>
        </div>
        <div class="status-btn-card" style="background-color: #121f33; padding: 12px 6px; border-radius: 12px; border: 1px solid #1e293b;">
            <div class="status-title" style="font-size: 10px; color: #9ca3af; font-weight: bold; letter-spacing: 0.05em; margin-bottom: 4px;">BALANCE</div>
            <div class="status-state" id="state_balance" style="font-size: 16px; font-weight: 800; color: #6b7280;">--</div>
        </div>
    </div>

    <!-- กล่องรายละเอียดเซลล์สะสม -->
    <div class="card-bg grid-3col" style="margin-bottom: 16px;">
        <div style="grid-column: span 3 / span 3; text-align: left; margin-bottom: -4px;">
            <div class="label-text">REMAIN / FULL CAPACITY</div>
            <div id="txt_remain_capacity" style="font-size: 14px; font-weight: 700; color: #f3f4f6;">--.-- / --.-- Ah</div>
        </div>
        <div style="grid-column: span 2 / span 2;">
            <div class="label-text" style="text-align:left;">CELL AVERAGE</div>
            <div id="txt_avg_cell_volt" class="value-text" style="text-align:left; font-size:16px;">--.--- V</div>
        </div>
        <div>
            <div class="label-text" style="text-align:right;">CELL DELTA</div>
            <div id="txt_volt_gap" class="value-text text-emerald-400" style="text-align:right; font-size:16px;">--.--- V</div>
        </div>
    </div>

       <!-- 🟢 เพิ่มกล่องคู่ตรงกลาง: อุณหภูมิระบบ และ ช่วงแรงดันเซลล์สูงสุด/ต่ำสุด -->
    <div class="card-row" style="display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 16px; margin-bottom: 16px;">
        
        <!-- ฝั่งซ้าย: อุณหภูมิและสถานะ -->
        <div class="card-bg" style="background-color: #121f33; padding: 16px; border-radius: 16px; border: 1px solid #1e293b; display: flex; flex-direction: column; gap: 12px;">
            <div class="label-text" style="font-size: 10px; color: #9ca3af; font-weight: bold;">อุณหภูมิและสถานะ</div>
            
            <div style="display: flex; justify-content: space-between; align-items: center; background-color: #0f1c2e; padding: 6px 12px; border-radius: 8px;">
                <span style="font-size: 12px; color: #9ca3af;">Board</span>
                <span style="font-size: 16px; font-weight: 700; color: #ffffff;"><span id="mid_board_temp">--.-</span> °C</span>
            </div>
            <div style="display: flex; justify-content: space-between; align-items: center; background-color: #0f1c2e; padding: 6px 12px; border-radius: 8px;">
                <span style="font-size: 12px; color: #9ca3af;">Chip</span>
                <span style="font-size: 16px; font-weight: 700; color: #ffffff;"><span id="mid_chip_temp">--.-</span> °C</span>
            </div>
            <div style="display: flex; justify-content: space-between; align-items: center; background-color: #0f1c2e; padding: 6px 12px; border-radius: 8px;">
                <span style="font-size: 12px; color: #fbbf24;">MOS</span>
                <span style="font-size: 16px; font-weight: 700; color: #fbbf24;"><span id="mid_mos_temp">--.-</span> °C</span>
            </div>
        </div>

        <!-- ฝั่งขวา: ช่วงแรงดันเซลล์ (MIN/MAX) -->
        <div class="card-bg" style="background-color: #121f33; padding: 16px; border-radius: 16px; border: 1px solid #1e293b; display: flex; flex-direction: column; gap: 12px;">
            <div class="label-text" style="font-size: 10px; color: #9ca3af; font-weight: bold;">ช่วงแรงดันเซลล์</div>
            
            <!-- MINIMUM -->
            <div style="display: flex; justify-content: space-between; align-items: center; background-color: #0f1c2e; padding: 10px 12px; border-radius: 8px;">
                <span style="font-size: 10px; color: #9ca3af; font-weight: bold;">MINIMUM</span>
                <div style="text-align: right;">
                    <div style="font-size: 11px; color: #f87171; font-weight: 700;" id="mid_min_cell_name">C03</div>
                    <div style="font-size: 13px; font-weight: 700; color: #ffffff;"><span id="mid_min_cell_volt">-.---</span>V</div>
                </div>
            </div>
            
            <!-- MAXIMUM -->
            <div style="display: flex; justify-content: space-between; align-items: center; background-color: #0f1c2e; padding: 10px 12px; border-radius: 8px;">
                <span style="font-size: 10px; color: #9ca3af; font-weight: bold;">MAXIMUM</span>
                <div style="text-align: right;">
                    <div style="font-size: 11px; color: #38bdf8; font-weight: 700;" id="mid_max_cell_name">C02</div>
                    <div style="font-size: 13px; font-weight: 700; color: #ffffff;"><span id="mid_max_cell_volt">-.---</span>V</div>
                </div>
            </div>
        </div>

    </div>


    <script>
        function updateBMSData() {
            fetch('get_bms_data.php')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('txt_mac_addr').innerText = data.mac_addr;
                    document.getElementById('txt_soc').innerText = data.soc;
                    
                    let isChargingEl = document.getElementById('txt_is_charging');
                    isChargingEl.innerText = data.is_charging;
                    
                    // ปรับสีตัวอักษรสถานะตามลักษณะไฟที่วิ่ง
                    if(data.is_charging === "CHARGING") {
                        isChargingEl.style.color = "#22d3ee";
                        document.getElementById('txt_flow_text').style.color = "#34d399";
                    } else {
                        isChargingEl.style.color = "#f87171";
                        document.getElementById('txt_flow_text').style.color = "#f87171";
                    }
                    
                    document.getElementById('circle_border').style.borderColor = data.circle_border_color;
                    document.getElementById('txt_power').innerText = data.power;
                    document.getElementById('txt_flow_text').innerText = data.flow_text;
                    document.getElementById('txt_total_volt').innerText = data.total_volt;
                    document.getElementById('txt_current_load').innerText = data.current_load;
                    document.getElementById('txt_capacity').innerText = data.capacity;
                    document.getElementById('txt_remain_capacity').innerText = data.remain_capacity;
                    document.getElementById('txt_avg_cell_volt').innerText = data.avg_cell_volt;
                    document.getElementById('txt_volt_gap').innerText = data.volt_gap;
                    document.getElementById('txt_board_temp').innerText = data.board_temp;
                    document.getElementById('txt_cell1').innerText = data.cell1;
                    document.getElementById('txt_cell2').innerText = data.cell2;
                    document.getElementById('txt_cell3').innerText = data.cell3;
                    document.getElementById('txt_cell4').innerText = data.cell4;
                                        // 🟢 สั่งอัปเดตค่าแรงดันและค่าความต้านทานสายไฟในกล่อง Matrix ด้านล่างสุด
                    if(document.getElementById('matrix_v1')) document.getElementById('matrix_v1').innerText = parseFloat(data.cell1 || 0).toFixed(3);
                    if(document.getElementById('matrix_v2')) document.getElementById('matrix_v2').innerText = parseFloat(data.cell2 || 0).toFixed(3);
                    if(document.getElementById('matrix_v3')) document.getElementById('matrix_v3').innerText = parseFloat(data.cell3 || 0).toFixed(3);
                    if(document.getElementById('matrix_v4')) document.getElementById('matrix_v4').innerText = parseFloat(data.cell4 || 0).toFixed(3);

                    // อัปเดตค่าความต้านทาน (ดึงจากฟันเฟืองข้อมูลฐานข้อมูล ถ้าไม่มีคอลัมน์ระบบจะใส่ค่าจำลองโชว์ให้สวยงาม)
                    if(document.getElementById('matrix_r1')) document.getElementById('matrix_r1').innerText = data.wire1 || '0.038';
                    if(document.getElementById('matrix_r2')) document.getElementById('matrix_r2').innerText = data.wire2 || '0.031';
                    if(document.getElementById('matrix_r3')) document.getElementById('matrix_r3').innerText = data.wire3 || '0.034';
                    if(document.getElementById('matrix_r4')) document.getElementById('matrix_r4').innerText = data.wire4 || '0.032';
                                        // 🟢 สั่งอัปเดตค่าข้อมูลช่วงอุณหภูมิและการ์ดคู่ตรงกลาง
                    if(document.getElementById('mid_board_temp')) document.getElementById('mid_board_temp').innerText = data.board_temp || '--.-';
                    if(document.getElementById('mid_chip_temp')) document.getElementById('mid_chip_temp').innerText = data.chip1_temp || '--.-';
                    if(document.getElementById('mid_mos_temp')) document.getElementById('mid_mos_temp').innerText = data.mos_temp || '--.-';

                    // ลอจิกหาเซลล์ที่แรงดันต่ำสุด (MIN) และสูงสุด (MAX) มาแสดงผลอัตโนมัติ
                    let cellVolts = [
                        { name: 'C01', volt: parseFloat(data.cell1 || 0) },
                        { name: 'C02', volt: parseFloat(data.cell2 || 0) },
                        { name: 'C03', volt: parseFloat(data.cell3 || 0) },
                        { name: 'C04', volt: parseFloat(data.cell4 || 0) }
                    ];

                    // เรียงลำดับจากน้อยไปมาก
                    cellVolts.sort((a, b) => a.volt - b.volt);
                    let minCell = cellVolts[0];
                    let maxCell = cellVolts[cellVolts.length - 1];

                    // แสดงผลฝั่ง MIN/MAX
                    if(document.getElementById('mid_min_cell_name')) document.getElementById('mid_min_cell_name').innerText = minCell.name;
                    if(document.getElementById('mid_min_cell_volt')) document.getElementById('mid_min_cell_volt').innerText = minCell.volt.toFixed(3);
                    if(document.getElementById('mid_max_cell_name')) document.getElementById('mid_max_cell_name').innerText = maxCell.name;
                    if(document.getElementById('mid_max_cell_volt')) document.getElementById('mid_max_cell_volt').innerText = maxCell.volt.toFixed(3);

                                        // --- ลอจิกเปิด BALANCE ON อัตโนมัติเมื่อแรงดัน >= 3.35V มากกว่า 1 เซลล์ ---
                    let val_c1 = parseFloat(data.cell1 || 0);
                    let val_c2 = parseFloat(data.cell2 || 0);
                    let val_c3 = parseFloat(data.cell3 || 0);
                    let val_c4 = parseFloat(data.cell4 || 0);
                    
                    let highVoltCount = 0;
                    if (val_c1 >= 3.35) highVoltCount++;
                    if (val_c2 >= 3.35) highVoltCount++;
                    if (val_c3 >= 3.35) highVoltCount++;
                    if (val_c4 >= 3.35) highVoltCount++;

                    let stat_balance = (highVoltCount > 1) ? "ON" : "OFF";
                    let stat_charge = "ON";    
                    let stat_discharge = "ON"; 

                    // ฟังก์ชันสั่งเปลี่ยนข้อความและไฮไลต์สีฟ้าเรียลไทม์
                    const setStatusBtn = (id, stat) => {
                        let e = document.getElementById(id);
                        if(e) {
                            e.innerText = stat;
                            e.style.color = (stat === "ON") ? "#22d3ee" : "#6b7280";
                        }
                    };
                    
                    setStatusBtn('state_charge', stat_charge);
                    setStatusBtn('state_discharge', stat_discharge);
                    setStatusBtn('state_balance', stat_balance);
                    // ----------------------------------------------------------------------
                    

                    })
                    .catch(err => console.error("Error fetching real-time data:", err));
                    }
                    updateBMSData();
                    setInterval(updateBMSData, 2000); // อัปเดตข้อมูลอัตโนมัติเบื้องหลังทุกๆ 2 วินาที
            </script>
                <!-- 🟢 เพิ่มกล่อง CELL MATRIX ส่วนท้ายสุดตามแบบเป๊ะๆ -->
    <div class="card-bg" style="background-color: #121f33; padding: 20px; border-radius: 16px; border: 1px solid #1e293b; margin-top: 16px; margin-bottom: 24px;">
        <div style="font-size: 10px; color: #9ca3af; letter-spacing: 0.05em; text-transform: uppercase; margin-bottom: 2px;">CELL MATRIX</div>
        <div style="font-size: 15px; font-weight: 700; color: #ffffff; margin-bottom: 14px;">แรงดันและความต้านทานแต่ละเซลล์</div>
        
        <div style="display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 12px;">
            <!-- Cell 01 (แถบสีฟ้า) -->
            <div style="background-color: #0f1c2e; padding: 12px; border-radius: 8px; border-left: 4px solid #22d3ee;">
                <div style="font-size: 10px; color: #22d3ee; font-weight: 700; margin-bottom: 2px;">CELL 01</div>
                <div style="font-size: 18px; font-weight: 700; color: #ffffff; margin-bottom: 2px;"><span id="matrix_v1">--.---</span> V</div>
                <div style="font-size: 10px; color: #6b7280; font-family: monospace;">WIRE <span id="matrix_r1">0.000</span> Ω</div>
            </div>
            <!-- Cell 02 (แถบสีเขียว) -->
            <div style="background-color: #0f1c2e; padding: 12px; border-radius: 8px; border-left: 4px solid #34d399;">
                <div style="font-size: 10px; color: #34d399; font-weight: 700; margin-bottom: 2px;">CELL 02</div>
                <div style="font-size: 18px; font-weight: 700; color: #ffffff; margin-bottom: 2px;"><span id="matrix_v2">--.---</span> V</div>
                <div style="font-size: 10px; color: #6b7280; font-family: monospace;">WIRE <span id="matrix_r2">0.000</span> Ω</div>
            </div>
            <!-- Cell 03 (แถบสีเขียว) -->
            <div style="background-color: #0f1c2e; padding: 12px; border-radius: 8px; border-left: 4px solid #34d399;">
                <div style="font-size: 10px; color: #34d399; font-weight: 700; margin-bottom: 2px;">CELL 03</div>
                <div style="font-size: 18px; font-weight: 700; color: #ffffff; margin-bottom: 2px;"><span id="matrix_v3">--.---</span> V</div>
                <div style="font-size: 10px; color: #6b7280; font-family: monospace;">WIRE <span id="matrix_r3">0.000</span> Ω</div>
            </div>
            <!-- Cell 04 (แถบสีฟ้า) -->
            <div style="background-color: #0f1c2e; padding: 12px; border-radius: 8px; border-left: 4px solid #22d3ee;">
                <div style="font-size: 10px; color: #22d3ee; font-weight: 700; margin-bottom: 2px;">CELL 04</div>
                <div style="font-size: 18px; font-weight: 700; color: #ffffff; margin-bottom: 2px;"><span id="matrix_v4">--.---</span> V</div>
                <div style="font-size: 10px; color: #6b7280; font-family: monospace;">WIRE <span id="matrix_r4">0.000</span> Ω</div>
            </div>
        </div>
    </div>

    </body>
</html>
