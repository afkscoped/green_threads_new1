import React, { useState, useEffect, useRef } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, AreaChart, Area } from 'recharts';
import { Play, Pause, RefreshCw, Settings, Activity, Server, Cpu, Zap } from 'lucide-react';
import useWebSocket from 'react-use-websocket';

// --- Premium UI Components ---

const Card = ({ children, className = "" }) => (
    <div className={`bg-slate-900/50 backdrop-blur-xl border border-white/10 rounded-2xl p-6 shadow-2xl ${className}`}>
        {children}
    </div>
);

const Badge = ({ active, children }) => (
    <span className={`px-3 py-1 rounded-full text-xs font-bold tracking-wider transition-all duration-300 ${active
        ? 'bg-green-500/20 text-green-400 border border-green-500/30 shadow-[0_0_15px_rgba(34,197,94,0.3)]'
        : 'bg-blue-500/20 text-blue-400 border border-blue-500/30 shadow-[0_0_15px_rgba(59,130,246,0.3)]'
        }`}>
        {children}
    </span>
);

const Button = ({ onClick, children, variant = 'primary', icon: Icon, className = "" }) => {
    const baseStyle = "flex items-center justify-center gap-2 px-6 py-3 rounded-xl font-bold transition-all duration-200 active:scale-95 disabled:opacity-50 disabled:cursor-not-allowed";
    const variants = {
        primary: "bg-gradient-to-r from-emerald-500 to-teal-600 hover:from-emerald-400 hover:to-teal-500 text-white shadow-lg shadow-emerald-900/40 border border-white/10",
        danger: "bg-gradient-to-r from-red-500 to-rose-600 hover:from-red-400 hover:to-rose-500 text-white shadow-lg shadow-red-900/40 border border-white/10",
        secondary: "bg-slate-800 hover:bg-slate-700 text-slate-200 border border-white/5",
        accent: "bg-indigo-600 hover:bg-indigo-500 text-white border border-indigo-400/30"
    };

    return (
        <button onClick={onClick} className={`${baseStyle} ${variants[variant]} ${className}`}>
            {Icon && <Icon size={18} />}
            <span>{children}</span>
        </button>
    );
};

const Slider = ({ label, value, min, max, onChange, onCommit, accentColor = "green" }) => (
    <div className="space-y-4">
        <div className="flex justify-between items-center">
            <label className="text-sm font-medium text-slate-400">{label}</label>
            <span className={`text-lg font-mono font-bold ${accentColor === 'green' ? 'text-emerald-400' : 'text-amber-400'}`}>
                {value}
            </span>
        </div>
        <input
            type="range" min={min} max={max}
            value={value}
            onChange={(e) => onChange(e.target.value)}
            onMouseUp={onCommit}
            className={`w-full h-1.5 bg-slate-700 rounded-lg appearance-none cursor-pointer accent-${accentColor === 'green' ? 'emerald' : 'amber'}-500 hover:accent-${accentColor === 'green' ? 'emerald' : 'amber'}-400 transition-all`}
        />
    </div>
);

// --- Main Dashboard ---

const Dashboard = () => {
    const [threads, setThreads] = useState([]);
    const [stats, setStats] = useState({ switches: 0, active: 0, mode: 'green' });
    const [history, setHistory] = useState([]);

    // Control State
    const [timeSlice, setTimeSlice] = useState(10);
    const [tickets, setTickets] = useState(10);
    const [workloadSize, setWorkloadSize] = useState(20);

    // Ref to track previous metric values for rate calculation
    const prevMetrics = useRef(null);
    const lastUpdateTime = useRef(Date.now());

    const { lastMessage, readyState } = useWebSocket('ws://localhost:3001', {
        shouldReconnect: () => true,
        reconnectAttempts: 10,
        reconnectInterval: 1000,
    });

    useEffect(() => {
        if (lastMessage !== null) {
            try {
                const data = JSON.parse(lastMessage.data);

                if (data.type === 'metrics') {
                    const green = data.green;
                    const pthread = data.pthread;
                    const now = Date.now();

                    setStats({
                        switches: green.switches + pthread.switches,
                        active: green.active + pthread.active,
                        mode: green.active > 0 ? 'green' : 'mixed'
                    });

                    // Calculate rate (delta / time_delta_seconds) 
                    let greenSwitchRate = 0;
                    let pthreadSwitchRate = 0;
                    let greenWorkRate = 0;
                    let pthreadWorkRate = 0;

                    if (prevMetrics.current) {
                        const timeDeltaSec = (now - lastUpdateTime.current) / 1000;
                        if (timeDeltaSec > 0) {
                            greenSwitchRate = (green.switches - prevMetrics.current.green.switches) / timeDeltaSec;
                            pthreadSwitchRate = (pthread.switches - prevMetrics.current.pthread.switches) / timeDeltaSec;
                            greenWorkRate = (green.work - prevMetrics.current.green.work) / timeDeltaSec;
                            pthreadWorkRate = (pthread.work - prevMetrics.current.pthread.work) / timeDeltaSec;
                        }
                    }

                    // Store current as previous for next iteration
                    prevMetrics.current = { green, pthread };
                    lastUpdateTime.current = now;

                    setHistory(h => {
                        const timeStr = new Date().toLocaleTimeString();
                        const next = [...h, {
                            time: timeStr,
                            green_switches: Math.max(0, greenSwitchRate),
                            pthread_switches: Math.max(0, pthreadSwitchRate),
                            green_work: Math.max(0, greenWorkRate),
                            pthread_work: Math.max(0, pthreadWorkRate),
                        }];
                        // Keep last 60 points
                        return next.slice(-60);
                    });
                }
            } catch (e) {
                console.error("Parse error", e);
            }
        }
    }, [lastMessage]);

    const sendCommand = async (cmd) => {
        console.log("TX:", cmd);
        try {
            await fetch('http://localhost:3001/api/control', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ command: cmd })
            });
        } catch (e) {
            console.error("Failed to send command", e);
        }
    };

    const handleSliceChange = () => sendCommand(`set:slice:${timeSlice}`);
    const handleTicketsChange = () => sendCommand(`set:tickets:${tickets}`);

    // Separate Spawn Commands
    const spawnGreen = () => sendCommand(`spawn:green:${workloadSize}`);
    const spawnPthread = () => sendCommand(`spawn:pthread:${workloadSize}`);
    const stopWorkload = () => sendCommand('spawn:matrix:0'); // Stops all

    const runSweep = async () => {
        console.log("Starting Sweep...");
        for (let t = 5; t <= 100; t += 10) {
            setTickets(t);
            sendCommand(`set:tickets:${t}`);
            await new Promise(r => setTimeout(r, 800));
        }
    };

    const connectionStatus = {
        0: 'Connecting...',
        1: 'Connected',
        2: 'Closing',
        3: 'Closed'
    }[readyState];

    return (
        <div className="min-h-screen bg-[#0B0F19] text-slate-200 font-sans selection:bg-emerald-500/30">
            {/* Background Gradients */}
            <div className="fixed inset-0 pointer-events-none overflow-hidden">
                <div className="absolute top-[-10%] left-[-10%] w-[40%] h-[40%] bg-emerald-500/10 rounded-full blur-[120px]" />
                <div className="absolute bottom-[-10%] right-[-10%] w-[40%] h-[40%] bg-blue-500/10 rounded-full blur-[120px]" />
            </div>

            <div className="relative z-10 p-6 max-w-[1600px] mx-auto space-y-8">

                {/* Header */}
                <header className="flex justify-between items-end pb-6 border-b border-white/5">
                    <div className="space-y-2">
                        <div className="flex items-center gap-3">
                            <div className="bg-emerald-500/20 p-2 rounded-lg">
                                <Activity className="text-emerald-400" size={24} />
                            </div>
                            <h1 className="text-3xl font-bold text-transparent bg-clip-text bg-gradient-to-r from-emerald-400 to-cyan-400">
                                GreenThreads++
                            </h1>
                        </div>
                        <p className="text-slate-400 text-sm font-medium pl-1">Research-Grade Runtime Observability</p>
                    </div>

                    <div className="flex items-center gap-6">
                        <div className="text-right">
                            <div className="text-xs font-bold text-slate-500 uppercase tracking-widest mb-1">Status</div>
                            <div className="flex items-center gap-2">
                                <div className={`w-2 h-2 rounded-full ${readyState === 1 ? 'bg-emerald-400 shadow-[0_0_10px_#34d399]' : 'bg-red-400'}`} />
                                <span className="font-mono text-sm">{connectionStatus}</span>
                            </div>
                        </div>
                        <div className="h-8 w-px bg-white/10 mx-2" />
                        <div className="text-right">
                            <div className="text-xs font-bold text-slate-500 uppercase tracking-widest mb-1">Total Switches</div>
                            <div className="text-2xl font-mono font-bold text-white">{stats.switches.toLocaleString()}</div>
                        </div>
                        <div className="text-right">
                            <div className="text-xs font-bold text-slate-500 uppercase tracking-widest mb-1">Active Threads</div>
                            <div className="text-2xl font-mono font-bold text-emerald-400">{stats.active}</div>
                        </div>
                    </div>
                </header>

                {/* Main Grid */}
                <div className="grid grid-cols-1 lg:grid-cols-12 gap-8">

                    {/* Control Panel */}
                    <Card className="lg:col-span-4 space-y-8 h-fit">
                        <div>
                            <h2 className="text-xl font-bold text-white mb-6 flex items-center gap-2">
                                <Settings className="text-slate-400" size={20} />
                                <span>Runtime Control</span>
                            </h2>

                            <div className="space-y-8">
                                <Slider
                                    label="Scheduler Time Slice (ms)"
                                    value={timeSlice} min={1} max={100}
                                    onChange={setTimeSlice} onCommit={handleSliceChange}
                                    accentColor="green"
                                />
                                <Slider
                                    label="Stride Tickets (Priority)"
                                    value={tickets} min={1} max={100}
                                    onChange={setTickets} onCommit={handleTicketsChange}
                                    accentColor="amber"
                                />

                                <div className="space-y-3">
                                    <label className="text-sm font-medium text-slate-400">Workload Complexity (N²)</label>
                                    <div className="flex gap-4">
                                        <input
                                            type="number" min="1" max="100"
                                            value={workloadSize} onChange={(e) => setWorkloadSize(e.target.value)}
                                            className="bg-slate-800 border border-slate-700 rounded-lg px-4 py-2 w-24 text-center font-mono focus:border-emerald-500 outline-none transition"
                                        />
                                        <span className="text-xs text-slate-500 self-center">Threads</span>
                                    </div>
                                </div>
                            </div>
                        </div>

                        {/* Control Panel Update */}
                        <div className="pt-6 border-t border-white/10 space-y-4">
                            <div className="grid grid-cols-2 gap-4">
                                <Button onClick={spawnGreen} variant="primary" icon={Play}>
                                    Spawn Green
                                </Button>
                                <Button onClick={spawnPthread} variant="secondary" icon={Play}>
                                    Spawn Pthread
                                </Button>
                            </div>
                            <Button onClick={stopWorkload} variant="danger" icon={Pause} className="w-full">
                                Stop All Workloads
                            </Button>
                            <Button onClick={runSweep} variant="accent" icon={RefreshCw} className="w-full">
                                Run Ticket Sweep Experiment
                            </Button>
                        </div>
                    </Card>

                    {/* Charts Column */}
                    <div className="lg:col-span-8 flex flex-col gap-8">

                        {/* Context Switches Chart */}
                        <Card className="flex flex-col min-h-[400px]">
                            <div className="flex justify-between items-center mb-6">
                                <h2 className="text-xl font-bold text-white flex items-center gap-2">
                                    <Zap className="text-slate-400" size={20} />
                                    <span>Context Switches / Sec</span>
                                </h2>
                                <div className="flex gap-2">
                                    <Badge active={true}>GREEN THREADS</Badge>
                                    <Badge active={false}>PTHREADS</Badge>
                                </div>
                            </div>

                            <div className="flex-1 w-full min-h-0">
                                <ResponsiveContainer width="100%" height="100%">
                                    <AreaChart data={history}>
                                        <defs>
                                            <linearGradient id="colorGreen" x1="0" y1="0" x2="0" y2="1">
                                                <stop offset="5%" stopColor="#34d399" stopOpacity={0.3} />
                                                <stop offset="95%" stopColor="#34d399" stopOpacity={0} />
                                            </linearGradient>
                                            <linearGradient id="colorBlue" x1="0" y1="0" x2="0" y2="1">
                                                <stop offset="5%" stopColor="#3b82f6" stopOpacity={0.3} />
                                                <stop offset="95%" stopColor="#3b82f6" stopOpacity={0} />
                                            </linearGradient>
                                        </defs>
                                        <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" vertical={false} />
                                        <XAxis dataKey="time" stroke="#475569" tick={{ fill: '#475569' }} tickLine={false} axisLine={false} />
                                        <YAxis stroke="#475569" tick={{ fill: '#475569' }} tickLine={false} axisLine={false} width={40} />
                                        <Tooltip contentStyle={{ backgroundColor: '#0f172a', border: '1px solid #1e293b', borderRadius: '12px', color: '#fff' }} itemStyle={{ color: '#fff' }} />
                                        <Area type="monotone" dataKey="green_switches" stroke="#34d399" strokeWidth={3} fillOpacity={1} fill="url(#colorGreen)" activeDot={{ r: 6, strokeWidth: 0 }} />
                                        <Area type="monotone" dataKey="pthread_switches" stroke="#3b82f6" strokeWidth={3} fillOpacity={1} fill="url(#colorBlue)" />
                                    </AreaChart>
                                </ResponsiveContainer>
                            </div>
                        </Card>

                        {/* CPU Efficiency */}
                        <Card className="flex flex-col min-h-[400px]">
                            <div className="flex justify-between items-center mb-6">
                                <div className="space-y-1">
                                    <h2 className="text-xl font-bold text-white flex items-center gap-2">
                                        <Cpu className="text-slate-400" size={20} />
                                        <span>Real CPU Efficiency</span>
                                    </h2>
                                    <p className="text-xs text-slate-500 font-mono">Matrix Ops / Second (Strictly time-sliced)</p>
                                </div>
                            </div>

                            <div className="flex-1 w-full min-h-0">
                                <ResponsiveContainer width="100%" height="100%">
                                    <LineChart data={history}>
                                        <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" vertical={false} />
                                        <XAxis dataKey="time" stroke="#475569" tick={{ fill: '#475569' }} tickLine={false} axisLine={false} />
                                        <YAxis stroke="#475569" tick={{ fill: '#475569' }} tickLine={false} axisLine={false} width={40} />
                                        <Tooltip contentStyle={{ backgroundColor: '#0f172a', border: '1px solid #1e293b', borderRadius: '12px', color: '#fff' }} itemStyle={{ color: '#fff' }} />
                                        <Line type="monotone" dataKey="green_work" stroke="#34d399" strokeWidth={3} dot={false} />
                                        <Line type="monotone" dataKey="pthread_work" stroke="#3b82f6" strokeWidth={3} dot={false} />
                                    </LineChart>
                                </ResponsiveContainer>
                            </div>
                        </Card>
                    </div>
                </div>

                <footer className="text-center text-slate-600 text-sm mt-12">
                    GreenThreads++ v2.0 • Running on Alpine Linux Container
                </footer>
            </div>
        </div>
    );
};

export default Dashboard;
