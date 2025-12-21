function spawn(type) {
    fetch('/spawn?type=' + type, { method: 'POST' })
        .then(res => res.text())
        .then(msg => console.log('Spawn:', msg))
        .catch(err => console.error('Spawn failed:', err));
}

function updateTable() {
    fetch('/status')
        .then(response => response.json())
        .then(data => {
            const tbody = document.getElementById('task-table');
            tbody.innerHTML = ''; // Clear existing

            data.sort((a, b) => a.id - b.id);

            data.forEach(task => {
                const tr = document.createElement('tr');

                // State mapping
                // 0: RUNNABLE, 1: SLEEPING, 2: WAITING, 3: DONE
                let stateStr = 'UNKNOWN';
                let stateClass = '';
                if (task.state === 0) { stateStr = 'RUNNABLE'; stateClass = 'runnable'; }
                else if (task.state === 1) { stateStr = 'SLEEPING'; stateClass = 'sleeping'; }
                else if (task.state === 2) { stateStr = 'WAITING'; stateClass = 'waiting'; }
                else if (task.state === 3) { stateStr = 'DONE'; stateClass = 'done'; }

                // Info column
                let info = '';
                if (task.state === 1 && task.wake_ms > 0) {
                    const remaining = Math.max(0, task.wake_ms - Date.now());
                    info = `Wakes in ${remaining}ms`;
                } else if (task.type === 'CPU') {
                    info = `<div class="progress-bar"><div class="fill" style="width:${task.progress}%"></div></div> ${task.progress}%`;
                } else if (task.state === 2) {
                    info = `Waiting on FD: ${task.wait_fd}`;
                }

                tr.innerHTML = `
                    <td>${task.id}</td>
                    <td>${task.type}</td>
                    <td class="${stateClass}">${stateStr}</td>
                    <td>${info}</td>
                `;
                tbody.appendChild(tr);
            });
        })
        .catch(err => console.error('Error fetching status:', err));
}

setInterval(updateTable, 500);
updateTable();
