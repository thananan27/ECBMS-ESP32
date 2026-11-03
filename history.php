<?php
$servername = "localhost"; 
$username = "root"; 
$password = "1qaz2wsx"; 
$dbname = "m5stack_db";

$conn = new mysqli($servername, $username, $password, $dbname);
if ($conn->connect_error) { die("Database connection failed"); }
$conn->set_charset("utf8");

$range = $_GET['range'] ?? '1h';

if ($range == '24h') { 
    $limit_count = 150; 
} elseif ($range == '7d') { 
    $limit_count = 300; 
} else { 
    $limit_count = 60; 
}

// คิวรี่ดึงข้อมูลตัวล่าสุดจาก MariaDB
$sql = "SELECT * FROM (
            SELECT created_at, total_volt, soc 
            FROM bms_records 
            ORDER BY id DESC 
            LIMIT $limit_count
        ) AS sub_table 
        ORDER BY created_at ASC";

$result = $conn->query($sql);
$labels = []; $volts = []; $socs = [];

if ($result && $result->num_rows > 0) {
    while($row = $result->fetch_assoc()) {
        $labels[] = date('H:i', strtotime($row['created_at']));
        $volts[] = (float)$row['total_volt'];
        $socs[] = (int)$row['soc'];
    }
}
?>
<!DOCTYPE html>
<html lang="th">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>BMS History - สถิติระบบย้อนหลัง</title>
    <!-- เรียกสคริปต์วาดกราฟจากไฟล์ในเครื่องออฟไลน์ -->
    <script src="chart.min.js"></script>
    <style>
        body { 
            background-color: #0d1624; 
            color: #ffffff; 
            font-family: sans-serif; 
            padding: 16px; 
            max-width: 450px; 
            margin: auto; 
            box-sizing: border-box;
        }
        /* เมนูหลักด้านบนสุด */
        .menu-grid { 
            display: flex !important; 
            flex-direction: row !important;
            justify-content: space-between !important;
            align-items: center !important;
            width: 100% !important;
            gap: 6px !important; 
            margin-bottom: 24px !important; 
        }
        .menu-item {
            display: block !important;
            flex: 1 !important;
            background-color: #111827; 
            color: #9ca3af; 
            padding: 10px 0; 
            border-radius: 6px; 
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
        /* หัวข้อหน้าจอ */
        .header-title { 
            font-size: 18px; 
            font-weight: 700; 
            margin-bottom: 16px; 
            color: #f3f4f6;
            border-left: 4px solid #06b6d4;
            padding-left: 8px;
        }
        /* ตัวกรองช่วงเวลา */
        .filter-container {
            display: flex;
            gap: 8px;
            margin-bottom: 20px;
            width: 100%;
        }
        .filter-btn {
            flex: 1;
            display: block;
            padding: 10px 0;
            border-radius: 10px;
            text-decoration: none;
            text-align: center;
            font-size: 12px;
            font-weight: 500;
        }
        .filter-active {
            background-color: #083344;
            border: 1px solid #06b6d4;
            color: #22d3ee;
        }
        .filter-inactive {
            background-color: #111827;
            border: 1px solid #1f293d;
            color: #9ca3af;
        }
        /* กล่องการ์ดสำหรับใส่กราฟ */
        .card-bg { 
            background-color: #121f33; 
            padding: 16px; 
            border-radius: 16px; 
            margin-bottom: 16px;
            border: 1px solid #1e293b;
        }
        .chart-title {
            font-size: 10px;
            color: #9ca3af;
            font-weight: 600;
            margin-bottom: 12px;
            letter-spacing: 0.05em;
            font-family: monospace;
        }
    </style>
</head>
<body>

    <!-- แถบเมนูบาร์หลักเรียงแนวนอนหน้ากระดาน -->
    <div class="menu-grid">
        <a href="dashboard.php" class="menu-item">BOARD</a>
        <span class="menu-item menu-active">GRAPH</span>
        <a href="report.php" class="menu-item">HOURLY</a>
        <span class="menu-item">REPORT</span>
        <span class="menu-item">SETUP</span>
    </div>
    
    <h2 class="header-title">สถิติระบบย้อนหลัง</h2>
    
    <!-- กลุ่มปุ่มกดเลือกช่วงเวลา -->
    <div class="filter-container">
        <a href="?range=1h" class="filter-btn <?= $range=='1h'?'filter-active':'filter-inactive';?>">1 ชั่วโมง</a>
        <a href="?range=24h" class="filter-btn <?= $range=='24h'?'filter-active':'filter-inactive';?>">24 ชั่วโมง</a>
        <a href="?range=7d" class="filter-btn <?= $range=='7d'?'filter-active':'filter-inactive';?>">7 วัน</a>
    </div>
    
    <!-- กล่องแสดงกราฟแรงดันรวม -->
    <div class="card-bg">
        <div class="chart-title">TOTAL VOLTAGE PROFILE (V)</div>
        <canvas id="voltChart" style="width:100%; max-height:160px;"></canvas>
    </div>
    
    <!-- กล่องแสดงกราฟเปอร์เซ็นต์แบตเตอรี่ -->
    <div class="card-bg">
        <div class="chart-title">STATE OF CHARGE (%)</div>
        <canvas id="socChart" style="width:100%; max-height:160px;"></canvas>
    </div>

<script>
    const timeLabels = <?= json_encode($labels, JSON_HEX_TAG | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_HEX_AMP); ?>;
    const voltageData = <?= json_encode($volts, JSON_NUMERIC_CHECK); ?>;
    const socData = <?= json_encode($socs, JSON_NUMERIC_CHECK); ?>;

    const xAxisConfig = {
        grid: { display: false },
        ticks: {
            color: '#9ca3af',
            font: { size: 9 },
            maxRotation: 0,
            autoSkip: true,
            maxTicksLimit: 6
        }
    };

    const yAxisConfig = {
        grid: { color: '#1e293b' },
        ticks: {
            color: '#9ca3af',
            font: { size: 9 }
        }
    };

    // ตั้งค่ากลาง ป้องกัน undefined จากชื่อ Dataset และ Tooltip
    const commonOptions = {
        responsive: true,
        maintainAspectRatio: true,
        plugins: {
            legend: {
                display: false
            },
            title: {
                display: false,
                text: ''
            },
            tooltip: {
                enabled: true,
                callbacks: {
                    title: function(items) {
                        if (!items || items.length === 0) return '';
                        return items[0].label || '';
                    },
                    label: function(context) {
                        const value = context.parsed.y;
                        if (value === null || value === undefined ||
                            !Number.isFinite(value)) {
                            return '';
                        }

                        if (context.dataset.label === 'Total Voltage') {
                            return 'Voltage: ' + value.toFixed(2) + ' V';
                        }

                        if (context.dataset.label === 'Battery SOC') {
                            return 'SOC: ' + value + ' %';
                        }

                        return String(value);
                    }
                }
            }
        },
        scales: {
            x: xAxisConfig,
            y: yAxisConfig
        }
    };

    // กราฟแรงดันรวม
    new Chart(document.getElementById('voltChart'), {
        type: 'line',
        data: {
            labels: timeLabels,
            datasets: [{
                label: 'Total Voltage',
                data: voltageData,
                borderColor: '#22d3ee',
                backgroundColor: 'rgba(34,211,238,0.05)',
                fill: true,
                borderWidth: 2,
                pointRadius: 0,
                spanGaps: false
            }]
        },
        options: commonOptions
    });

    // กราฟเปอร์เซ็นต์แบตเตอรี่
    new Chart(document.getElementById('socChart'), {
        type: 'line',
        data: {
            labels: timeLabels,
            datasets: [{
                label: 'Battery SOC',
                data: socData,
                borderColor: '#4ade80',
                backgroundColor: 'rgba(74,222,128,0.05)',
                fill: true,
                borderWidth: 2,
                pointRadius: 0,
                spanGaps: false
            }]
        },
        options: {
            ...commonOptions,
            scales: {
                x: xAxisConfig,
                y: {
                    ...yAxisConfig,
                    min: 0,
                    max: 100
                }
            }
        }
    });
</script>

</body>
</html>
<?php $conn->close(); ?>
