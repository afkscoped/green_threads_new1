// Poll every 500ms
setInterval(fetchData, 500);

function fetchData() {
    fetch('/threads')
        .then(res => res.json())
        .then(data => {
            updateScheduler(data);
            updateStacks(data);
            updateMetrics(data);
        })
        .catch(err => {
            document.getElementById('connectionStatus').innerText = 'Disconnected';
            document.getElementById('connectionStatus').style.backgroundColor = 'red';
            console.error(err);
        });
}

function updateScheduler(threads) {
    document.getElementById('connectionStatus').innerText = 'Connected';
    document.getElementById('connectionStatus').style.backgroundColor = '#4caf50';

    const tbody = document.querySelector('#schedulerTable tbody');
    tbody.innerHTML = '';

    threads.forEach(t => {
        const tr = document.createElement('tr');

        // State Map
        const states = ['RUNNABLE', 'SLEEPING', 'WAITING', 'DONE', 'NEW', 'RUNNING', 'BLOCKED', 'TERMINATED'];
        const stateStr = states[t.state] || t.state;

        // CPU Share Control: Input + Button
        const controlHtml = `
            <div class="control-group">
                <input type="number" id="tix-${t.id}" value="${t.tickets}" min="1" max="1000" style="width: 60px;">
                <button onclick="setTickets(${t.id})">Set</button>
            </div>
        `;

        tr.innerHTML = `
            <td>${t.id}</td>
            <td><span class="badge ${stateStr}">${stateStr}</span></td>
            <td>${t.tickets}</td>
            <td>${t.pass}</td>
            <td>${t.stride}</td>
            <td>${controlHtml}</td>
        `;
        tbody.appendChild(tr);
    });
}

function updateStacks(threads) {
    const container = document.getElementById('stackContainer');
    container.innerHTML = '';

    threads.forEach(t => {
        if (t.state === 3 || t.state === 7) return; // Skip DONE/TERMINATED

        const used = t.stack_used || 0;
        const total = 64 * 1024; // Default 64KB
        const pct = Math.min(100, Math.max(0, (used / total) * 100));

        let color = '#4caf50';
        if (pct > 70) color = '#ff9800';
        if (pct > 90) color = '#f44336';

        const div = document.createElement('div');
        div.className = 'stack-item';
        div.innerHTML = `
            <div class="stack-label">Thread ${t.id} Stack: ${used} / ${total} bytes (${pct.toFixed(1)}%)</div>
            <div class="stack-track">
                <div class="stack-fill" style="width: ${pct}%; background-color: ${color};"></div>
            </div>
        `;
        container.appendChild(div);
    });
}

function updateMetrics(threads) {
    // Runnable, Sleeping, Waiting
    let runnable = 0, sleeping = 0, waiting = 0;

    let ioHtml = '';

    threads.forEach(t => {
        if (t.state === 0 || t.state === 5) runnable++; // READY/RUNNING
        else if (t.state === 1) sleeping++;
        else if (t.state === 2 || t.state === 6) waiting++;

        if (t.waiting_fd >= 0) {
            ioHtml += `<div>Thread ${t.id} waiting on FD ${t.waiting_fd}</div>`;
        }
    });

    document.getElementById('statRunnable').innerText = runnable;
    document.getElementById('statSleep').innerText = sleeping;
    document.getElementById('statWait').innerText = waiting;

    const ioContainer = document.getElementById('ioList');
    ioContainer.innerHTML = ioHtml || 'None';
}

window.setTickets = function (tid) {
    const input = document.getElementById(`tix-${tid}`);
    const val = input.value;

    fetch('/tickets', {
        method: 'POST',
        body: `id=${tid}&tickets=${val}`
    }).then(() => fetchData()); // Refresh immediately
};
