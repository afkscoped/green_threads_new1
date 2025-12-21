setInterval(fetchData, 500);

function fetchData() {
    fetch('/threads')
        .then(res => res.json())
        .then(data => {
            updateDashboard(data);
            document.getElementById('connectionStatus').innerText = 'Connected';
            document.getElementById('connectionStatus').style.backgroundColor = '#4caf50';
        })
        .catch(err => {
            document.getElementById('connectionStatus').innerText = 'Disconnected';
            document.getElementById('connectionStatus').style.backgroundColor = '#f44336';
        });
}

function updateDashboard(threads) {
    updateStride(threads);
    updateStacks(threads);
    updateSync(threads);
    updateSleep(threads);
    updateIO(threads);
    updateMetrics(threads);
}

function updateStride(threads) {
    const tbody = document.querySelector('#schedulerTable tbody');
    tbody.innerHTML = '';

    // States: 0=NEW, 1=READY, 2=RUNNING, 3=BLOCKED, 4=TERMINATED (approx)
    // Actually enum: NEW=0, READY=1, RUNNING=2, BLOCKED=3, TERMINATED=4
    const states = ['NEW', 'READY', 'RUNNING', 'BLOCKED', 'TERMINATED'];

    threads.forEach(t => {
        if (t.state === 4) return; // Skip terminated in stride view if desired, or show them

        const tr = document.createElement('tr');
        const stateStr = states[t.state] || t.state;

        tr.innerHTML = `
            <td>${t.id}</td>
            <td><span class="badge ${stateStr}">${stateStr}</span></td>
            <td>${t.tickets}</td>
            <td>${t.pass}</td>
            <td>${t.stride}</td>
            <td>
                <input type="number" id="tix-${t.id}" value="${t.tickets}" style="width:50px">
                <button onclick="setTickets(${t.id})">Set</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function updateStacks(threads) {
    const container = document.getElementById('stackContainer');
    container.innerHTML = '';
    threads.forEach(t => {
        if (t.state === 4) return;
        const used = t.stack_used || 0;
        const total = 64 * 1024;
        const pct = Math.min(100, (used / total) * 100);

        let color = '#4caf50';
        if (pct > 70) color = '#ff9800';
        if (pct > 90) color = '#f44336';

        container.innerHTML += `
            <div class="stack-item">
                <div class="stack-label">ID ${t.id}: ${used} B</div>
                <div class="stack-track" style="background:#ddd;height:10px;border-radius:5px;overflow:hidden">
                    <div style="width:${pct}%;background:${color};height:100%"></div>
                </div>
            </div>`;
    });
}

function updateIO(threads) {
    const list = document.getElementById('ioList');
    const waiting = threads.filter(t => t.waiting_fd >= 0 && t.state !== 4);

    if (waiting.length === 0) {
        list.innerHTML = 'No I/O waiting threads.';
        return;
    }

    list.innerHTML = waiting.map(t =>
        `<div class="sub-panel">Thread <strong>${t.id}</strong> blocked on FD <strong>${t.waiting_fd}</strong></div>`
    ).join('');
}

function updateSleep(threads) {
    const list = document.getElementById('sleepList');
    // We assume wake_time > 0 means sleeping (and state BLOCKED)
    // Need current time to show "Seconds remaining"? Frontend doesn't know server time.
    // Just show wake timestamp.
    const sleeping = threads.filter(t => t.wake_time > 0 && t.state !== 4);

    if (sleeping.length === 0) {
        list.innerHTML = 'No sleeping threads.';
        return;
    }

    list.innerHTML = sleeping.map(t =>
        `<div class="sub-panel">Thread <strong>${t.id}</strong> sleeping until <strong>${t.wake_time}</strong></div>`
    ).join('');
}

function updateSync(threads) {
    const list = document.getElementById('syncList');

    // Threads blocked but NOT on IO and NOT on Sleep
    const blocked = threads.filter(t =>
        t.state === 3 && // BLOCKED
        t.waiting_fd === -1 &&
        (t.wake_time === 0 || t.wake_time === undefined)
    );

    if (blocked.length === 0) {
        list.innerHTML = 'No threads blocked on Mutex/Sync.';
        return;
    }

    list.innerHTML = blocked.map(t =>
        `<div class="sub-panel" style="border-left: 4px solid #f44336;">
            Thread <strong>${t.id}</strong> BLOCKED (Mutex/Condition)
        </div>`
    ).join('');
}

function updateMetrics(threads) {
    let runnable = 0, sleeping = 0, waiting = 0;
    threads.forEach(t => {
        if (t.state === 1 || t.state === 2) runnable++;
        else if (t.wake_time > 0) sleeping++;
        else if (t.state === 3) waiting++;
    });
    document.getElementById('statRunnable').innerText = runnable;
    document.getElementById('statSleep').innerText = sleeping;
    document.getElementById('statWait').innerText = waiting;
}

window.setTickets = function (id) {
    const val = document.getElementById(`tix-${id}`).value;
    fetch('/tickets', { method: 'POST', body: `id=${id}&tickets=${val}` });
};
