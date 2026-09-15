const fallbackModel = {
  schema: 1,
  screen: { width: 800, height: 480, columns: 3 },
  status: { mode: 'passive-monitor', bus: 'RS485 listening', discovery: 'pending' },
  settings: {
    observation: { mode: 'read-only', baud: 9600, parity: 'none', stopBits: 1, address: 'discovery pending' },
    display: { brightness: 80, rotation: 0, touch: true },
    outputs: { enabled: false, reason: 'green-terminal outputs require explicit hardware approval' },
    wifi: { connected: false, ssid: 'not configured', deviceName: 'haier-controller', ip: '192.168.4.1', rssi: null, accessPoint: 'haier-hmi' },
    ota: { channel: 'stable', version: 'v0.1.0-dev', updateAvailable: false, manifestStatus: 'not checked' },
    commissioning: { stage: 'idle', sensorType: 'XY-MD02', role: 'upstairs', currentAddress: 1, targetAddress: 24, lastResult: 'No commissioning attempt' }
  },
  tiles: [
    { id: 'outdoor', label: 'Outdoor', unit: '°C', value: '8.4', humidity: '82.1', kind: 'temperature', availability: 'available', ageSeconds: 4, detail: 'XY-MD02 · fresh 4 s' },
    { id: 'upstairs', label: 'Upstairs', unit: '°C', value: '21.6', target: '21.0', humidity: '48.3', kind: 'room', availability: 'available', ageSeconds: 4, detail: 'XY-MD02 · fresh 4 s' },
    { id: 'downstairs', label: 'Downstairs', unit: '°C', value: '20.9', target: '21.0', humidity: '51.7', kind: 'room', availability: 'available', ageSeconds: 4, detail: 'XY-MD02 · fresh 4 s' },
    { id: 'dhw', label: 'DHW tank', unit: '°C', value: '48.2', target: '55', kind: 'temperature', detail: 'sensor source pending' },
    { id: 'flow', label: 'Flow', unit: '°C', value: '42.7', target: '45', kind: 'water', detail: 'heating circuit' },
    { id: 'return', label: 'Return', unit: '°C', value: '34.1', kind: 'water', detail: 'heating circuit' },
    { id: 'flow-rate', label: 'Flow rate', unit: 'L/min', value: '8.6', kind: 'rate', detail: 'calculation source pending' },
    { id: 'thermal-output', label: 'Thermal output', unit: 'kW out', value: '5.4', kind: 'energy', detail: 'derived from flow delta' },
    { id: 'thermal-input', label: 'Thermal input', unit: 'kW in', value: '6.1', kind: 'energy', detail: 'Roomstat data pending' }
  ]
};

const tileGrid = document.querySelector('.tile-grid');
const status = document.querySelector('.status');
const footer = document.querySelector('footer');
const pageTitle = document.querySelector('#page-title');
const settingsForm = document.querySelector('#settings-form');
const brightnessValue = document.querySelector('#brightness-value');
const saveState = document.querySelector('#save-state');
const wifiState = document.querySelector('#wifi-state');
const otaState = document.querySelector('#ota-state');
const commissioningState = document.querySelector('#commissioning-state');
const settingsDetailTitle = document.querySelector('#settings-detail-title');
const wifiNetworkList = document.querySelector('#wifi-network-list');

function renderTile(tile) {
  const article = document.createElement('article');
  article.className = `tile ${tile.id} ${tile.kind}`;
  article.dataset.tileId = tile.id;
  const unavailableLabel = tile.availability === 'not-configured' ? 'Sensor not configured' : 'Sensor comms error';
  const state = tile.availability && tile.availability !== 'available' ? unavailableLabel : tile.detail;
  const humidity = tile.humidity === undefined || tile.humidity === null ? '' : `<em class="humidity">${Number(tile.humidity).toFixed(1)}% RH</em>`;
  const target = tile.target === undefined || tile.target === null ? '' : `<span class="target"> / ${tile.target}</span>`;
  article.innerHTML = `<span class="label">${tile.label}${humidity}</span><strong>${tile.value}${target}</strong><span class="unit">${tile.unit}</span><small>${state}</small>`;
  return article;
}

function render(model) {
  tileGrid.replaceChildren(...model.tiles.map(renderTile));
  status.innerHTML = `<span class="dot"></span> ${model.status.mode.replace('-', ' ')} <span class="divider"></span> ${model.status.bus}`;
  footer.replaceChildren(
    Object.assign(document.createElement('span'), { textContent: 'Last bus frame: 14:32:08' }),
    Object.assign(document.createElement('span'), { textContent: `Roomstat: ${model.status.discovery}` }),
    Object.assign(document.createElement('span'), { textContent: `${model.tiles.length} signals` })
  );
  settingsForm.elements.mode.value = model.settings.observation.mode;
  settingsForm.elements.baud.value = model.settings.observation.baud;
  settingsForm.elements.parity.value = model.settings.observation.parity;
  settingsForm.elements.stopBits.value = model.settings.observation.stopBits;
  settingsForm.elements.brightness.value = model.settings.display.brightness;
  settingsForm.elements.touch.checked = model.settings.display.touch;
  brightnessValue.value = `${model.settings.display.brightness}%`;
  document.querySelector('#device-address').textContent = model.settings.observation.address;
  document.querySelector('#output-note').textContent = model.settings.outputs.reason;
  document.querySelector('#wifi-ssid').value = model.settings.wifi.ssid;
  document.querySelector('#wifi-device-name').value = model.settings.wifi.deviceName;
  wifiState.textContent = model.settings.wifi.connected ? `Connected · ${model.settings.wifi.ip}` : `Access point · ${model.settings.wifi.accessPoint}`;
  document.querySelector('#ota-channel').value = model.settings.ota.channel;
  otaState.textContent = `${model.settings.ota.version} · ${model.settings.ota.manifestStatus}`;
  document.querySelector('#commissioning-role').value = model.settings.commissioning.role;
  document.querySelector('#commissioning-address').value = model.settings.commissioning.currentAddress;
  document.querySelector('#commissioning-target').value = model.settings.commissioning.targetAddress;
  commissioningState.textContent = `${model.settings.commissioning.stage} · ${model.settings.commissioning.lastResult}`;
}

document.querySelectorAll('[data-view]').forEach((button) => {
  button.addEventListener('click', () => {
    const view = button.dataset.view;
    document.querySelectorAll('[data-view]').forEach((item) => item.classList.toggle('active', item === button));
    document.querySelectorAll('[data-panel]').forEach((panel) => {
      const active = panel.dataset.panel === view;
      panel.hidden = !active;
      panel.classList.toggle('active', active);
    });
    pageTitle.textContent = view === 'settings' ? 'Controller settings' : 'System overview';
  });
});

const settingTitles = {
  wifi: 'Wi-Fi Settings',
  ota: 'OTA Updates',
  commissioning: 'RS485 Sensor Commissioning',
  display: 'Display and Touch',
  outputs: 'Field Outputs'
};

document.querySelectorAll('[data-setting]').forEach((button) => {
  button.addEventListener('click', () => {
    const setting = button.dataset.setting;
    settingsDetailTitle.textContent = settingTitles[setting];
    document.querySelectorAll('[data-setting-panel]').forEach((panel) => {
      panel.hidden = panel.dataset.settingPanel !== setting;
    });
    document.querySelector('[data-panel="settings"]').hidden = true;
    document.querySelector('[data-panel="settings-detail"]').hidden = false;
    pageTitle.textContent = settingTitles[setting];
  });
});

document.querySelector('#settings-back').addEventListener('click', () => {
  document.querySelector('[data-panel="settings-detail"]').hidden = true;
  document.querySelector('[data-panel="settings"]').hidden = false;
  pageTitle.textContent = 'Controller settings';
});

settingsForm.elements.brightness.addEventListener('input', (event) => {
  brightnessValue.value = `${event.target.value}%`;
});

settingsForm.addEventListener('submit', (event) => {
  event.preventDefault();
  saveState.textContent = 'Preview settings saved locally';
  window.setTimeout(() => { saveState.textContent = ''; }, 2200);
});

document.querySelector('#wifi-scan').addEventListener('click', async () => {
  wifiState.textContent = 'Scanning nearby networks...';
  try {
    const start = await fetch('/api/wifi/scan', {method: 'POST'});
    if (!start.ok) throw new Error('scan unavailable');
    let response = await fetch('/api/wifi/scan');
    let result = await response.json();
    for (let attempt = 0; result.busy && attempt < 12; attempt += 1) {
      await new Promise((resolve) => window.setTimeout(resolve, 500));
      response = await fetch('/api/wifi/scan');
      result = await response.json();
    }
    if (!response.ok) throw new Error('scan unavailable');
    wifiNetworkList.replaceChildren(...(result.networks || []).map((network) => {
      const option = document.createElement('option');
      option.value = network.ssid;
      option.textContent = `${network.ssid} (${network.rssi} dBm)`;
      return option;
    }));
    wifiNetworkList.hidden = !(result.networks || []).length;
    wifiState.textContent = result.busy
      ? 'Scan still running · try again shortly'
      : `Scan complete · ${(result.networks || []).length} networks found`;
  } catch (error) {
    wifiState.textContent = 'Preview only · device scan unavailable';
  }
});

wifiNetworkList.addEventListener('change', (event) => {
  document.querySelector('#wifi-ssid').value = event.target.value;
});

document.querySelector('#wifi-connect').addEventListener('click', async () => {
  const payload = {
    ssid: document.querySelector('#wifi-ssid').value.trim(),
    password: document.querySelector('#wifi-password').value,
    deviceName: document.querySelector('#wifi-device-name').value.trim()
  };
  if (!payload.ssid) {
    wifiState.textContent = 'Enter or scan for a network first';
    return;
  }
  try {
    const response = await fetch('/api/wifi/connect', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(payload)
    });
    if (!response.ok) throw new Error('connect unavailable');
    const result = await response.json();
    wifiState.textContent = `${result.status} · keep haier-hmi available while connecting`;
  } catch (error) {
    wifiState.textContent = 'Preview only · device API not connected';
  }
});

document.querySelector('#ota-check').addEventListener('click', () => {
  otaState.textContent = 'Preview only · manifest check simulated';
});

document.querySelector('#ota-apply').addEventListener('click', () => {
  otaState.textContent = 'Preview only · update not applied';
});

async function commission(step) {
  const payload = {
    step,
    address: Number(document.querySelector('#commissioning-address').value),
    target_address: Number(document.querySelector('#commissioning-target').value),
    temperature_offset: Number(document.querySelector('#commissioning-temperature-offset').value),
    humidity_offset: Number(document.querySelector('#commissioning-humidity-offset').value)
  };
  commissioningState.textContent = `${step} · contacting device...`;
  try {
    const response = await fetch('/api/modbus/commission', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(payload)
    });
    const result = await response.json();
    commissioningState.textContent = `${step} · ${result.message || response.statusText}`;
  } catch (error) {
    commissioningState.textContent = 'Preview only · device API not connected';
  }
}

document.querySelector('#commissioning-start').addEventListener('click', () => commission('detect'));
document.querySelector('#commissioning-change').addEventListener('click', () => commission('change'));
document.querySelector('#commissioning-verify').addEventListener('click', () => commission('verify'));
document.querySelector('#commissioning-calibrate').addEventListener('click', () => commission('calibrate'));

fetch('../common/dashboard/dashboard_model.json')
  .catch(() => fetch('./dashboard_model.json'))
  .then((response) => response.ok ? response.json() : Promise.reject(new Error('shared model unavailable')))
  .then(render)
  .catch(() => render(fallbackModel));
