/**
  ******************************************************************************
  * AQUASYNC ROV TELEMETRY & ATTITUDE SUITE - JAVASCRIPT CORE
  * WebSerial Engine, 3D Artificial Horizon, Oscilloscope & Telemetry Parser
  * STM32F401RCT6
  ******************************************************************************
  */

// --- STATE MANAGEMENT ---
const state = {
  connected: false,
  isSimulating: false,
  port: null,
  reader: null,
  baudRate: 115200,
  packetsReceived: 0,
  lastPacketTime: performance.now(),
  packetHz: 0,
  maxDepth: 0.0,
  lastDepth: 0.0,
  lastDepthTime: performance.now(),
  climbRate: 0.0,
  surfacePressure: 1013.25,
  activeChartChannel: 'depth',
  autoscroll: true,
  isRecording: false,
  recordedData: [],
  history: {
    timestamps: [],
    depth: [],
    pitch: [],
    roll: [],
    yaw: [],
    ax: [], ay: [], az: [],
    mx: [], my: [], mz: []
  },
  currentData: {
    ts: 0,
    gy521: { ax: 0, ay: 0, az: 1, gx: 0, gy: 0, gz: 0, roll: 0, pitch: 0 },
    gy87:  { ax: 0, ay: 0, az: 1, gx: 0, gy: 0, gz: 0, mx: 15, my: -8, mz: 42, hdg: 0, press: 1013.2, alt: 0 },
    ms5540: { press: 1013.2, temp: 22.5, depth: 0.0 }
  }
};

const MAX_HISTORY = 120;

// --- DOM ELEMENTS ---
const connectBtn = document.getElementById('connectBtn');
const simToggle = document.getElementById('simToggle');
const baudRateSelect = document.getElementById('baudRate');
const statusPill = document.getElementById('statusPill');
const statusText = document.getElementById('statusText');
const metricHz = document.getElementById('metricHz');
const metricPackets = document.getElementById('metricPackets');
const metricDepth = document.getElementById('metricDepth');
const metricHeading = document.getElementById('metricHeading');
const metricTemp = document.getElementById('metricTemp');
const metricUptime = document.getElementById('metricUptime');
const recordBtn = document.getElementById('recordBtn');
const tareBtn = document.getElementById('tareBtn');

const pitchVal = document.getElementById('pitchVal');
const rollVal = document.getElementById('rollVal');
const yawVal = document.getElementById('yawVal');

const depthNumber = document.getElementById('depthNumber');
const pressureNumber = document.getElementById('pressureNumber');
const climbRateNumber = document.getElementById('climbRateNumber');
const maxDepthNumber = document.getElementById('maxDepthNumber');
const waterFill = document.getElementById('waterFill');
const rovDepthMarker = document.getElementById('rovDepthMarker');
const markerDepthTag = document.getElementById('markerDepthTag');
const depthRuler = document.getElementById('depthRuler');

const compassDial = document.getElementById('compassDial');
const headingOverlay = document.getElementById('headingOverlay');
const barMx = document.getElementById('barMx');
const barMy = document.getElementById('barMy');
const barMz = document.getElementById('barMz');
const valMx = document.getElementById('valMx');
const valMy = document.getElementById('valMy');
const valMz = document.getElementById('valMz');

const gy521Accel = document.getElementById('gy521Accel');
const gy521Gyro = document.getElementById('gy521Gyro');
const gy521Att = document.getElementById('gy521Att');
const gy87Accel = document.getElementById('gy87Accel');
const gy87Gyro = document.getElementById('gy87Gyro');
const gy87Alt = document.getElementById('gy87Alt');

const terminalOutput = document.getElementById('terminalOutput');
const autoscrollBtn = document.getElementById('autoscrollBtn');
const clearTermBtn = document.getElementById('clearTermBtn');
const exportLogBtn = document.getElementById('exportLogBtn');

const horizonCanvas = document.getElementById('horizonCanvas');
const horizonCtx = horizonCanvas.getContext('2d');
const telemetryChart = document.getElementById('telemetryChart');
const chartCtx = telemetryChart.getContext('2d');

// --- INITIALIZATION ---
function initDepthRuler() {
  depthRuler.innerHTML = '';
  for (let m = 0; m <= 20; m += 4) {
    const mark = document.createElement('span');
    mark.textContent = `${m}m`;
    depthRuler.appendChild(mark);
  }
}
initDepthRuler();

// --- LOGGING UTILITY ---
function logToTerminal(msg, type = 'info') {
  const line = document.createElement('div');
  line.className = `term-line ${type}`;
  line.textContent = msg;
  terminalOutput.appendChild(line);

  // Keep max 200 lines
  if (terminalOutput.childNodes.length > 200) {
    terminalOutput.removeChild(terminalOutput.firstChild);
  }

  if (state.autoscroll) {
    terminalOutput.scrollTop = terminalOutput.scrollHeight;
  }
}

// --- DATA PROCESSING ---
function processTelemetryData(jsonObj) {
  state.packetsReceived++;
  state.currentData = jsonObj;

  const now = performance.now();
  const dt = (now - state.lastPacketTime) / 1000;
  state.lastPacketTime = now;
  if (dt > 0) {
    state.packetHz = (state.packetHz * 0.8) + ((1 / dt) * 0.2);
  }

  // Determine primary active sensor values (supports GY-521, GY-87, or both)
  const primaryPitch = (jsonObj.gy521 && jsonObj.gy521.pitch !== undefined && jsonObj.gy521.pitch !== 0) ? jsonObj.gy521.pitch :
                       (jsonObj.gy87 ? (jsonObj.gy87.pitch || 0) : 0);
  const primaryRoll  = (jsonObj.gy521 && jsonObj.gy521.roll !== undefined && jsonObj.gy521.roll !== 0) ? jsonObj.gy521.roll :
                       (jsonObj.gy87 ? (jsonObj.gy87.roll || 0) : 0);
  const primaryHdg   = (jsonObj.gy87 && jsonObj.gy87.hdg !== undefined) ? jsonObj.gy87.hdg : 0;

  // Calculate climb / descent rate (m/s)
  const depth = (jsonObj.ms5540 && jsonObj.ms5540.depth !== undefined && jsonObj.ms5540.depth !== 0) ? jsonObj.ms5540.depth :
                (jsonObj.gy87 && jsonObj.gy87.alt !== undefined && jsonObj.gy87.alt !== 0 ? Math.max(0, -jsonObj.gy87.alt) : 0);
  const depthDt = (now - state.lastDepthTime) / 1000;
  if (depthDt >= 0.2) {
    state.climbRate = (depth - state.lastDepth) / depthDt;
    state.lastDepth = depth;
    state.lastDepthTime = now;
  }

  if (depth > state.maxDepth) {
    state.maxDepth = depth;
  }

  // Push to history
  state.history.timestamps.push(now);
  state.history.depth.push(depth);
  state.history.pitch.push(primaryPitch);
  state.history.roll.push(primaryRoll);
  state.history.yaw.push(primaryHdg);
  state.history.ax.push(jsonObj.gy87 && jsonObj.gy87.ax !== undefined ? jsonObj.gy87.ax : (jsonObj.gy521 ? jsonObj.gy521.ax : 0));
  state.history.ay.push(jsonObj.gy87 && jsonObj.gy87.ay !== undefined ? jsonObj.gy87.ay : (jsonObj.gy521 ? jsonObj.gy521.ay : 0));
  state.history.az.push(jsonObj.gy87 && jsonObj.gy87.az !== undefined ? jsonObj.gy87.az : (jsonObj.gy521 ? jsonObj.gy521.az : 1));
  state.history.mx.push(jsonObj.gy87 ? (jsonObj.gy87.mx || 0) : 0);
  state.history.my.push(jsonObj.gy87 ? (jsonObj.gy87.my || 0) : 0);
  state.history.mz.push(jsonObj.gy87 ? (jsonObj.gy87.mz || 0) : 0);

  if (state.history.timestamps.length > MAX_HISTORY) {
    for (let key in state.history) {
      state.history[key].shift();
    }
  }

  if (state.isRecording) {
    state.recordedData.push({ ...jsonObj, clientTime: new Date().toISOString() });
  }

  updateUI();
}

// --- UI UPDATE ROUTINE ---
function updateUI() {
  const d = state.currentData;
  const gy521 = d.gy521 || {};
  const gy87 = d.gy87 || {};
  const ms5540 = d.ms5540 || {};

  // Resolved primary sensor metrics
  const activePitch = (gy521.pitch !== undefined && gy521.pitch !== 0) ? gy521.pitch : (gy87.pitch || 0);
  const activeRoll  = (gy521.roll  !== undefined && gy521.roll  !== 0) ? gy521.roll  : (gy87.roll  || 0);
  const activeHdg   = (gy87.hdg !== undefined) ? gy87.hdg : 0;
  const activeTemp  = (ms5540.temp && ms5540.temp !== 0) ? ms5540.temp :
                      ((gy87.mpu_temp && gy87.mpu_temp !== 0) ? gy87.mpu_temp :
                      ((gy521.temp && gy521.temp !== 0) ? gy521.temp : 22.0));
  const activeDepth = (ms5540.depth !== undefined && ms5540.depth !== 0) ? ms5540.depth :
                      ((gy87.alt !== undefined && gy87.alt !== 0) ? Math.max(0, -gy87.alt) : 0);
  const activePress = (ms5540.press && ms5540.press !== 0) ? ms5540.press : (gy87.press || 1013.2);

  // Quick bar
  metricHz.innerHTML = `${state.packetHz.toFixed(1)} <small>Hz</small>`;
  metricPackets.textContent = state.packetsReceived.toLocaleString();
  metricDepth.innerHTML = `${activeDepth.toFixed(2)} <small>m</small>`;
  metricHeading.innerHTML = `${activeHdg.toFixed(1)}° <small>${getCardinal(activeHdg)}</small>`;
  metricTemp.innerHTML = `${activeTemp.toFixed(1)} <small>°C</small>`;

  // Attitude readouts
  pitchVal.textContent = `${(activePitch >= 0 ? '+' : '')}${activePitch.toFixed(1)}°`;
  rollVal.textContent  = `${(activeRoll >= 0 ? '+' : '')}${activeRoll.toFixed(1)}°`;
  yawVal.textContent   = `${activeHdg.toFixed(1)}°`;

  // Depth Column
  depthNumber.textContent = activeDepth.toFixed(2);
  pressureNumber.textContent = activePress.toFixed(1);
  climbRateNumber.innerHTML = `${(state.climbRate >= 0 ? '+' : '')}${state.climbRate.toFixed(2)} <small>m/s</small>`;
  maxDepthNumber.textContent = state.maxDepth.toFixed(2);

  // Depth scale (0 to 20m)
  const depthPct = Math.min(100, Math.max(0, (activeDepth / 20.0) * 100));
  waterFill.style.height = `${depthPct}%`;
  rovDepthMarker.style.top = `${depthPct}%`;
  markerDepthTag.textContent = `${activeDepth.toFixed(1)}m`;

  // Compass Rose
  compassDial.style.transform = `rotate(${-activeHdg}deg)`;
  headingOverlay.textContent = `${activeHdg.toFixed(0).padStart(3, '0')}°`;

  // Magnetometer vector bars (-60 to +60 uT)
  updateMagBar(barMx, valMx, gy87.mx || 0);
  updateMagBar(barMy, valMy, gy87.my || 0);
  updateMagBar(barMz, valMz, gy87.mz || 0);

  // Dual IMU Cards
  gy521Accel.textContent = `${(gy521.ax || 0).toFixed(2)}, ${(gy521.ay || 0).toFixed(2)}, ${(gy521.az || 0).toFixed(2)}`;
  gy521Gyro.textContent  = `${(gy521.gx || 0).toFixed(1)}, ${(gy521.gy || 0).toFixed(1)}, ${(gy521.gz || 0).toFixed(1)}`;
  gy521Att.textContent   = `${(gy521.roll || 0).toFixed(1)}°, ${(gy521.pitch || 0).toFixed(1)}°`;

  gy87Accel.textContent = `${(gy87.ax || 0).toFixed(2)}, ${(gy87.ay || 0).toFixed(2)}, ${(gy87.az || 0).toFixed(2)}`;
  gy87Gyro.textContent  = `${(gy87.gx || 0).toFixed(1)}, ${(gy87.gy || 0).toFixed(1)}, ${(gy87.gz || 0).toFixed(1)}`;
  gy87Alt.textContent   = `${(gy87.alt || 0).toFixed(1)} m (${(gy87.press || 1013.2).toFixed(0)} hPa)`;
}

function updateMagBar(barEl, valEl, val) {
  valEl.textContent = `${val.toFixed(1)} µT`;
  const pct = Math.min(100, Math.max(0, ((val + 60) / 120) * 100));
  barEl.style.width = `${pct}%`;
}

function getCardinal(deg) {
  const directions = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
  const index = Math.round(((deg % 360) / 45)) % 8;
  return directions[index];
}

// --- ARTIFICIAL HORIZON CANVAS RENDERER ---
function drawArtificialHorizon() {
  const w = horizonCanvas.width;
  const h = horizonCanvas.height;
  const cx = w / 2;
  const cy = h / 2;

  const activePitch = (state.currentData.gy521 && state.currentData.gy521.pitch !== undefined && state.currentData.gy521.pitch !== 0) ? state.currentData.gy521.pitch :
                      (state.currentData.gy87 ? (state.currentData.gy87.pitch || 0) : 0);
  const activeRoll  = (state.currentData.gy521 && state.currentData.gy521.roll !== undefined && state.currentData.gy521.roll !== 0) ? state.currentData.gy521.roll :
                      (state.currentData.gy87 ? (state.currentData.gy87.roll || 0) : 0);

  const rollRad = (activeRoll * Math.PI) / 180;
  const pitchOffset = (activePitch / 90.0) * (h / 2);

  horizonCtx.clearRect(0, 0, w, h);

  // Save base state
  horizonCtx.save();

  // Clip to circular circle
  horizonCtx.beginPath();
  horizonCtx.arc(cx, cy, cx - 2, 0, Math.PI * 2);
  horizonCtx.clip();

  // Rotate & Translate for Roll & Pitch
  horizonCtx.translate(cx, cy);
  horizonCtx.rotate(-rollRad);
  horizonCtx.translate(0, pitchOffset);

  // Sky (Top Half)
  const skyGrad = horizonCtx.createLinearGradient(0, -h, 0, 0);
  skyGrad.addColorStop(0, '#0284c7');
  skyGrad.addColorStop(1, '#38bdf8');
  horizonCtx.fillStyle = skyGrad;
  horizonCtx.fillRect(-w * 2, -h * 2, w * 4, h * 2);

  // Ground / Water (Bottom Half)
  const seaGrad = horizonCtx.createLinearGradient(0, 0, 0, h);
  seaGrad.addColorStop(0, '#0f172a');
  seaGrad.addColorStop(1, '#020617');
  horizonCtx.fillStyle = seaGrad;
  horizonCtx.fillRect(-w * 2, 0, w * 4, h * 2);

  // Horizon Line
  horizonCtx.beginPath();
  horizonCtx.strokeStyle = '#00f2fe';
  horizonCtx.lineWidth = 3;
  horizonCtx.shadowColor = '#00f2fe';
  horizonCtx.shadowBlur = 10;
  horizonCtx.moveTo(-w * 2, 0);
  horizonCtx.lineTo(w * 2, 0);
  horizonCtx.stroke();
  horizonCtx.shadowBlur = 0;

  // Pitch Ladder Rungs
  horizonCtx.font = '10px JetBrains Mono';
  horizonCtx.fillStyle = '#ffffff';
  horizonCtx.strokeStyle = 'rgba(255, 255, 255, 0.7)';
  horizonCtx.lineWidth = 2;
  horizonCtx.textAlign = 'center';

  const pitchStepPx = (h / 2) / 90;
  for (let p = -80; p <= 80; p += 10) {
    if (p === 0) continue;
    const y = -p * pitchStepPx;
    const len = (Math.abs(p) % 20 === 0) ? 60 : 35;

    horizonCtx.beginPath();
    horizonCtx.moveTo(-len / 2, y);
    horizonCtx.lineTo(len / 2, y);
    horizonCtx.stroke();

    if (Math.abs(p) % 20 === 0) {
      horizonCtx.fillText(`${p}°`, -len / 2 - 16, y + 3);
      horizonCtx.fillText(`${p}°`, len / 2 + 16, y + 3);
    }
  }

  horizonCtx.restore();

  // Roll Arc Marks
  horizonCtx.save();
  horizonCtx.translate(cx, cy);
  horizonCtx.strokeStyle = 'rgba(56, 189, 248, 0.5)';
  horizonCtx.lineWidth = 2;

  const rollMarks = [-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60];
  const arcRadius = cx - 18;
  rollMarks.forEach(deg => {
    const rad = (deg - 90) * (Math.PI / 180);
    const x1 = Math.cos(rad) * arcRadius;
    const y1 = Math.sin(rad) * arcRadius;
    const x2 = Math.cos(rad) * (arcRadius - (deg % 30 === 0 ? 10 : 5));
    const y2 = Math.sin(rad) * (arcRadius - (deg % 30 === 0 ? 10 : 5));
    horizonCtx.beginPath();
    horizonCtx.moveTo(x1, y1);
    horizonCtx.lineTo(x2, y2);
    horizonCtx.stroke();
  });

  // Roll Pointer
  horizonCtx.rotate(-rollRad);
  horizonCtx.beginPath();
  horizonCtx.fillStyle = '#f59e0b';
  horizonCtx.moveTo(0, -(arcRadius - 12));
  horizonCtx.lineTo(-6, -(arcRadius - 22));
  horizonCtx.lineTo(6, -(arcRadius - 22));
  horizonCtx.closePath();
  horizonCtx.fill();

  horizonCtx.restore();
}

// --- REAL-TIME OSCILLOSCOPE CANVAS RENDERER ---
function drawTelemetryChart() {
  const w = telemetryChart.width;
  const h = telemetryChart.height;
  chartCtx.clearRect(0, 0, w, h);

  // Background Grid Lines
  chartCtx.strokeStyle = 'rgba(56, 189, 248, 0.08)';
  chartCtx.lineWidth = 1;
  const gridRows = 4;
  for (let i = 0; i <= gridRows; i++) {
    const y = (h / gridRows) * i;
    chartCtx.beginPath();
    chartCtx.moveTo(0, y);
    chartCtx.lineTo(w, y);
    chartCtx.stroke();
  }

  const series = state.history[state.activeChartChannel] || state.history.depth;
  if (!series || series.length < 2) return;

  // Determine min and max
  let min = Math.min(...series);
  let max = Math.max(...series);
  if (min === max) {
    min -= 1;
    max += 1;
  }
  const range = max - min;

  // Draw chart series
  chartCtx.beginPath();
  chartCtx.strokeStyle = '#00f2fe';
  chartCtx.lineWidth = 2.5;
  chartCtx.shadowColor = 'rgba(0, 242, 254, 0.5)';
  chartCtx.shadowBlur = 8;

  const stepX = w / (MAX_HISTORY - 1);
  const startIdx = MAX_HISTORY - series.length;

  for (let i = 0; i < series.length; i++) {
    const x = (startIdx + i) * stepX;
    const y = h - ((series[i] - min) / range) * (h - 20) - 10;
    if (i === 0) chartCtx.moveTo(x, y);
    else chartCtx.lineTo(x, y);
  }
  chartCtx.stroke();
  chartCtx.shadowBlur = 0;

  // Draw label overlay
  chartCtx.font = '11px JetBrains Mono';
  chartCtx.fillStyle = '#94a3b8';
  chartCtx.textAlign = 'right';
  chartCtx.fillText(`MAX: ${max.toFixed(2)}`, w - 10, 16);
  chartCtx.fillText(`MIN: ${min.toFixed(2)}`, w - 10, h - 10);
  chartCtx.textAlign = 'left';
  chartCtx.fillStyle = '#00f2fe';
  chartCtx.fillText(`CHANNEL: ${state.activeChartChannel.toUpperCase()}`, 12, 16);
}

// --- ANIMATION LOOP (60 FPS) ---
function animationLoop() {
  drawArtificialHorizon();
  drawTelemetryChart();

  // Uptime Counter
  const uptimeSec = Math.floor(performance.now() / 1000);
  const hrs = Math.floor(uptimeSec / 3600).toString().padStart(2, '0');
  const mins = Math.floor((uptimeSec % 3600) / 60).toString().padStart(2, '0');
  const secs = (uptimeSec % 60).toString().padStart(2, '0');
  metricUptime.textContent = `${hrs}:${mins}:${secs}`;

  requestAnimationFrame(animationLoop);
}
requestAnimationFrame(animationLoop);

// --- SIMULATION MODE ---
let simInterval = null;
function startSimulation() {
  logToTerminal('[SIM] Demo Simulation Activated! Generating subsea physics telemetry.', 'info');
  state.isSimulating = true;
  statusPill.classList.add('live');
  statusText.textContent = 'SIMULATION LIVE';

  let simTime = 0;
  simInterval = setInterval(() => {
    simTime += 0.05;

    // Realistic ROV movements
    const simPitch = Math.sin(simTime * 0.8) * 12.0;
    const simRoll  = Math.cos(simTime * 0.6) * 8.0;
    const simHdg   = (simTime * 5.0) % 360.0;
    const simDepth = Math.max(0, 8.0 + Math.sin(simTime * 0.3) * 6.0);
    const simPress = 1013.25 + (simDepth * 1000.0 * 9.80665) / 100.0;

    const sampleJSON = {
      ts: Math.floor(performance.now()),
      gy521: {
        ax: Math.sin(simRoll * Math.PI / 180),
        ay: -Math.sin(simPitch * Math.PI / 180),
        az: Math.cos(simRoll * Math.PI / 180) * Math.cos(simPitch * Math.PI / 180),
        gx: Math.cos(simTime) * 2.0,
        gy: Math.sin(simTime) * 3.0,
        gz: 0.5,
        roll: simRoll,
        pitch: simPitch
      },
      gy87: {
        ax: Math.sin(simRoll * Math.PI / 180),
        ay: -Math.sin(simPitch * Math.PI / 180),
        az: 1.0,
        gx: Math.cos(simTime) * 2.1,
        gy: Math.sin(simTime) * 2.9,
        gz: 0.5,
        mx: Math.cos(simHdg * Math.PI / 180) * 32.0,
        my: Math.sin(simHdg * Math.PI / 180) * 32.0,
        mz: 42.5,
        hdg: simHdg,
        press: simPress,
        alt: -simDepth
      },
      ms5540: {
        press: simPress,
        temp: 21.4 - (simDepth * 0.2),
        depth: simDepth
      }
    };

    processTelemetryData(sampleJSON);

    // Random debug line every 4 seconds
    if (Math.random() < 0.02) {
      logToTerminal(`[${Math.floor(performance.now())} ms] [INFO] Thruster stabilization active. Gyro DLPF locked.`, 'telem');
    }
  }, 50); // 20 Hz
}

function stopSimulation() {
  if (simInterval) clearInterval(simInterval);
  state.isSimulating = false;
  if (!state.connected) {
    statusPill.classList.remove('live');
    statusText.textContent = 'DISCONNECTED';
  }
  logToTerminal('[SIM] Demo Simulation Stopped.', 'warn');
}

simToggle.addEventListener('change', (e) => {
  if (e.target.checked) {
    startSimulation();
  } else {
    stopSimulation();
  }
});

// --- WEB SERIAL API CONNECTION ---
async function connectSerial() {
  if (!('serial' in navigator)) {
    alert('Web Serial API is not supported in this browser. Please open in Google Chrome, Microsoft Edge, or Opera!');
    return;
  }

  try {
    const selectedBaud = parseInt(baudRateSelect.value, 10);
    logToTerminal(`[SERIAL] Requesting USB Port at ${selectedBaud} baud...`, 'info');

    state.port = await navigator.serial.requestPort();
    await state.port.open({ baudRate: selectedBaud });

    state.connected = true;
    connectBtn.classList.add('connected');
    connectBtn.querySelector('.btn-text').textContent = 'Disconnect';
    statusPill.classList.add('live');
    statusText.textContent = 'CONNECTED';
    logToTerminal(`[SERIAL] Connected successfully at ${selectedBaud} baud! Streaming telemetry...`, 'info');

    // Turn off simulation if running
    if (state.isSimulating) {
      simToggle.checked = false;
      stopSimulation();
    }

    readSerialStream();
  } catch (err) {
    logToTerminal(`[SERIAL ERROR] ${err.message}`, 'error');
  }
}

async function disconnectSerial() {
  if (state.reader) {
    await state.reader.cancel();
  }
  if (state.port) {
    await state.port.close();
  }
  state.connected = false;
  connectBtn.classList.remove('connected');
  connectBtn.querySelector('.btn-text').textContent = 'Connect Serial';
  statusPill.classList.remove('live');
  statusText.textContent = 'DISCONNECTED';
  logToTerminal('[SERIAL] Port closed / Disconnected.', 'warn');
}

connectBtn.addEventListener('click', () => {
  if (state.connected) {
    disconnectSerial();
  } else {
    connectSerial();
  }
});

async function readSerialStream() {
  const textDecoder = new TextDecoderStream();
  const readableStreamClosed = state.port.readable.pipeTo(textDecoder.writable);
  state.reader = textDecoder.readable.getReader();

  let lineBuffer = '';

  try {
    while (true) {
      const { value, done } = await state.reader.read();
      if (done) break;
      if (value) {
        lineBuffer += value;
        const lines = lineBuffer.split('\n');
        lineBuffer = lines.pop(); // Retain incomplete line

        for (const line of lines) {
          const trimmed = line.trim();
          if (!trimmed) continue;

          if (trimmed.startsWith('{') && trimmed.endsWith('}')) {
            try {
              // Auto-sanitize if empty numeric values ever appear (e.g. "ax":, -> "ax":0,)
              const sanitized = trimmed.replace(/:\s*,/g, ':0,').replace(/:\s*}/g, ':0}');
              const data = JSON.parse(sanitized);
              processTelemetryData(data);
            } catch (e) {
              logToTerminal(`[MALFORMED JSON] ${trimmed}`, 'warn');
            }
          } else {
            // Text log line from STM32 LOGGER
            if (trimmed.includes('[ERROR]')) {
              logToTerminal(trimmed, 'error');
            } else if (trimmed.includes('[WARN]')) {
              logToTerminal(trimmed, 'warn');
            } else {
              logToTerminal(trimmed, 'info');
            }
          }
        }
      }
    }
  } catch (err) {
    logToTerminal(`[SERIAL READ EXCEPTION] ${err.message}`, 'error');
  } finally {
    state.reader.releaseLock();
  }
}

// --- BUTTON EVENT HANDLERS ---
tareBtn.addEventListener('click', () => {
  if (state.currentData.ms5540) {
    state.surfacePressure = state.currentData.ms5540.press;
    logToTerminal(`[TARE] Surface baseline tared to ${state.surfacePressure.toFixed(1)} mbar`, 'info');
  }
});

recordBtn.addEventListener('click', () => {
  state.isRecording = !state.isRecording;
  if (state.isRecording) {
    state.recordedData = [];
    recordBtn.classList.add('recording');
    recordBtn.innerHTML = '<span class="record-dot"></span> Stop Recording';
    logToTerminal('[RECORD] Started recording telemetry stream to buffer...', 'info');
  } else {
    recordBtn.classList.remove('recording');
    recordBtn.innerHTML = '<span class="record-dot"></span> Record Session';
    logToTerminal(`[RECORD] Stopped recording. Total ${state.recordedData.length} records captured.`, 'info');
    downloadJSON(state.recordedData, `rov_telemetry_${Date.now()}.json`);
  }
});

function downloadJSON(data, filename) {
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}

// Chart channel selection
document.querySelectorAll('.channel-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.channel-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    state.activeChartChannel = btn.dataset.chan;
  });
});

autoscrollBtn.addEventListener('click', () => {
  state.autoscroll = !state.autoscroll;
  autoscrollBtn.textContent = state.autoscroll ? 'Autoscroll ON' : 'Autoscroll OFF';
  autoscrollBtn.classList.toggle('active', state.autoscroll);
});

clearTermBtn.addEventListener('click', () => {
  terminalOutput.innerHTML = '';
});

exportLogBtn.addEventListener('click', () => {
  const text = Array.from(terminalOutput.querySelectorAll('.term-line'))
    .map(line => line.textContent)
    .join('\r\n');
  const blob = new Blob([text], { type: 'text/plain' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `rov_log_${Date.now()}.txt`;
  a.click();
  URL.revokeObjectURL(url);
});
