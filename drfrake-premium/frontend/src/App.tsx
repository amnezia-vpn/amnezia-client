// @ts-nocheck
import { useState, useEffect } from 'react';
import './App.css';
import { Auth } from './Auth';
import {
    Register, Login, Logout, GetCurrentUser,
    GetServers, Connect, Disconnect, IsConnected,
    GetSubscription, InitPayment, CheckPayment,
    CancelAutoRenew, EnableAutoRenew,
    GetPaymentHistory, GetPaymentMethod
} from '../wailsjs/go/main/App';
import { BrowserOpenURL } from '../wailsjs/runtime/runtime';

type ViewType = 'home' | 'servers' | 'pricing' | 'account';

function App() {
    const [view, setView] = useState<ViewType>('home');
    const [servers, setServers] = useState<any[]>([]);
    const [user, setUser] = useState<any>(null); // This is the API mock user info, separating from Auth user
    const [authUser, setAuthUser] = useState<any>(null); // The actual logged in user
    const [connected, setConnected] = useState(false);
    const [selectedServer, setSelectedServer] = useState<any>(null);
    const [status, setStatus] = useState('Disconnected');
    const [subscription, setSubscription] = useState<any>(null);
    const [payments, setPayments] = useState<any[]>([]);
    const [paymentMethod, setPaymentMethod] = useState<any>(null);
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        GetCurrentUser().then(u => {
            if (u) {
                setAuthUser(u);
                loadData();
            }
        });
    }, []);

    const loadData = async () => {
        try {
            const [srv, conn, sub, pm] = await Promise.all([
                GetServers(),
                IsConnected(),
                GetSubscription(),
                GetPaymentMethod(),
            ]);
            setServers(srv || []);
            setConnected(conn);
            setSubscription(sub);
            setPaymentMethod(pm);
        } catch (e) {
            console.error("Failed to load data:", e);
        }
    };

    if (!authUser) {
        return <Auth onLogin={(u) => { setAuthUser(u); loadData(); }} />;
    }

    const handleLogout = async () => {
        await Logout();
        setAuthUser(null);
        setView('home');
    };

    const toggleConnect = async () => {
        if (!selectedServer) {
            alert("Select a server first!");
            return;
        }

        if (connected) {
            setStatus('Disconnecting...');
            await Disconnect();
            setConnected(false);
            setStatus('Disconnected');
        } else {
            setStatus('Connecting...');
            try {
                await Connect(selectedServer.config, selectedServer.id);
                setConnected(true);
                setStatus('Connected');
            } catch (err: any) {
                alert("Connection failed: " + err);
                setStatus('Error');
            }
        }
    };

    const handlePayment = async (plan: string) => {
        setLoading(true);
        try {
            // 1. Init Payment
            const resp = await InitPayment(plan);
            if (!resp || !resp.confirmation_url) {
                alert("Failed to initialize payment");
                setLoading(false);
                return;
            }

            const paymentId = resp.id;
            const confirmationUrl = resp.confirmation_url;

            // 2. Open Browser
            BrowserOpenURL(confirmationUrl);

            // 3. Show loading state
            // Reusing loading state or create a new "paymentPending" state?
            // Better to use a modal or overlay. For now, simple alert + polling in background?
            // "Please complete payment in browser..."

            // Start Polling
            const pollInterval = setInterval(async () => {
                const status = await CheckPayment(paymentId);
                console.log("Payment status:", status);

                if (status === "succeeded") {
                    clearInterval(pollInterval);
                    alert("Payment Successful! Upgrading...");
                    await loadData();
                    setLoading(false);
                    setView('account'); // Changed from 'dashboard'
                } else if (status === "canceled") {
                    clearInterval(pollInterval);
                    alert("Payment Canceled.");
                    setLoading(false);
                }
            }, 3000);

            // Stop polling after 5 minutes
            setTimeout(() => {
                clearInterval(pollInterval);
                setLoading(false);
            }, 300000);

        } catch (e: any) {
            console.error(e);
            alert("Payment Error: " + String(e));
            setLoading(false);
        }
    };

    const handleUpgrade = (plan: string) => {
        handlePayment(plan);
    };

    const handleToggleAutoRenew = async () => {
        if (subscription?.autoRenew) {
            await CancelAutoRenew();
        } else {
            await EnableAutoRenew();
        }
        const sub = await GetSubscription();
        setSubscription(sub);
    };

    const handleSaveCard = async () => {
        await SavePaymentMethod("4242", "Visa", "12/28");
        const pm = await GetPaymentMethod();
        setPaymentMethod(pm);
        alert("Payment method saved!");
    };

    const daysRemaining = () => {
        if (!subscription?.expiryDate) return null;
        const diff = new Date(subscription.expiryDate).getTime() - Date.now();
        return Math.max(0, Math.ceil(diff / (1000 * 60 * 60 * 24)));
    };

    const isPremium = subscription && subscription.plan !== 'free';

    return (
        <div id="App">
            <aside className="sidebar">
                <div className="logo-area">DR. FRAKE</div>
                <nav className="nav-links">
                    {(['home', 'servers', 'pricing', 'account'] as ViewType[]).map(v => (
                        <div key={v} className={`nav-item ${view === v ? 'active' : ''}`} onClick={() => {
                            if (v === 'account') {
                                GetPaymentHistory().then(p => setPayments(p || []));
                            }
                            setView(v);
                        }}>
                            {v === 'home' ? '🏠 Home' : v === 'servers' ? '🌍 Locations' : v === 'pricing' ? '💎 Pricing' : '👤 Account'}
                        </div>
                    ))}
                    <div className="nav-item" onClick={handleLogout} style={{ marginTop: '2rem', color: '#ff4d4d' }}>
                        🚪 Logout
                    </div>
                </nav>
                <div style={{ marginTop: 'auto', padding: '1rem', borderTop: '1px solid rgba(255,255,255,0.1)' }}>
                    <div style={{ fontSize: '0.75rem', color: '#888', marginBottom: '0.3rem' }}>{authUser?.email}</div>
                    <div style={{
                        fontSize: '0.7rem',
                        color: isPremium ? '#00d7ff' : '#666',
                        fontWeight: isPremium ? 'bold' : 'normal'
                    }}>
                        {isPremium ? `⭐ ${String(subscription?.plan).toUpperCase()}` : 'Free Plan'}
                    </div>
                    {isPremium && daysRemaining() !== null && (
                        <div style={{ fontSize: '0.65rem', color: daysRemaining()! < 7 ? '#ff6b6b' : '#4CAF50', marginTop: '0.2rem' }}>
                            {daysRemaining()} days remaining
                        </div>
                    )}
                </div>
            </aside>

            <main className="main-content">
                {view === 'home' && (
                    <div className="dashboard">
                        {subscription?.status === 'grace' && (
                            <div className="grace-banner">
                                ⚠️ Your subscription has expired. You have 3 days to renew before losing premium access.
                                <button onClick={() => setView('pricing')} style={{ marginLeft: '1rem', color: '#00d7ff', background: 'none', border: '1px solid #00d7ff', borderRadius: '6px', padding: '4px 12px', cursor: 'pointer' }}>
                                    Renew Now
                                </button>
                            </div>
                        )}
                        <div className={`connect-hub ${connected ? 'connected' : ''}`} onClick={toggleConnect}>
                            <div className="outer-ring"></div>
                            <div className="inner-circle">
                                <div className={`status-dot ${connected ? 'online' : ''}`}></div>
                                <span style={{ fontSize: '1.2rem', fontWeight: 'bold' }}>{connected ? 'DISCONNECT' : 'CONNECT'}</span>
                                <span style={{ fontSize: '0.8rem', opacity: 0.7 }}>{status}</span>
                            </div>
                        </div>
                        <div style={{ marginTop: '3rem', textAlign: 'center' }}>
                            <h3>{selectedServer ? `${selectedServer.flag} ${selectedServer.country}` : 'No Server Selected'}</h3>
                            <p style={{ color: '#666' }}>Secure shadowsocks tunnel</p>
                        </div>
                    </div>
                )}

                {view === 'servers' && (
                    <div>
                        <h2 style={{ marginBottom: '2rem' }}>🌍 Global Servers</h2>
                        <div className="server-grid">
                            {servers.map(s => (
                                <div key={s.id} className={`server-card ${selectedServer?.id === s.id ? 'selected' : ''}`} onClick={() => {
                                    if (s.isPremium && !isPremium) {
                                        setView('pricing');
                                    } else {
                                        setSelectedServer(s);
                                        setView('home');
                                    }
                                }}>
                                    <div style={{ fontSize: '2rem' }}>{s.flag}</div>
                                    <div style={{ fontWeight: 'bold', margin: '0.5rem 0' }}>
                                        {s.city}, {s.country}
                                        {s.isPremium && <span className="badge">PREMIUM</span>}
                                    </div>
                                    <div style={{ fontSize: '0.8rem', color: s.latency < 80 ? '#00ff88' : '#ffaa00' }}>{s.latency} ms</div>
                                </div>
                            ))}
                        </div>
                    </div>
                )}

                {view === 'pricing' && (
                    <div style={{ textAlign: 'center' }}>
                        <h2 style={{ marginBottom: '0.5rem' }}>💎 Upgrade to Premium</h2>
                        <p style={{ color: '#888', marginBottom: '3rem' }}>Unlock all servers and get maximum speed</p>
                        <div style={{ display: 'flex', gap: '2rem', justifyContent: 'center', flexWrap: 'wrap' }}>
                            <div className="pricing-card">
                                <h3>Free</h3>
                                <div className="price">$0</div>
                                <ul className="features">
                                    <li>✅ 2 server locations</li>
                                    <li>✅ Basic speed</li>
                                    <li>❌ Premium locations</li>
                                    <li>❌ Priority support</li>
                                </ul>
                                <button className="btn-outline" disabled>Current</button>
                            </div>
                            <div className="pricing-card featured">
                                <div className="popular-tag">POPULAR</div>
                                <h3>Monthly</h3>
                                <div className="price">$9.99<span>/mo</span></div>
                                <ul className="features">
                                    <li>✅ All server locations</li>
                                    <li>✅ Maximum speed</li>
                                    <li>✅ Auto-renewal</li>
                                    <li>✅ Priority support</li>
                                </ul>
                                <button
                                    className="btn-primary"
                                    disabled={loading || subscription?.plan === 'monthly'}
                                    onClick={() => handleUpgrade('monthly')}
                                >
                                    {subscription?.plan === 'monthly' ? 'Active' : loading ? 'Processing...' : 'Subscribe'}
                                </button>
                            </div>
                            <div className="pricing-card">
                                <h3>Yearly</h3>
                                <div className="price">$79.99<span>/yr</span></div>
                                <div style={{ color: '#00ff88', fontSize: '0.8rem', marginBottom: '1rem' }}>Save 33%!</div>
                                <ul className="features">
                                    <li>✅ Everything in Monthly</li>
                                    <li>✅ 4 months FREE</li>
                                </ul>
                                <button
                                    className="btn-primary"
                                    disabled={loading || subscription?.plan === 'yearly'}
                                    onClick={() => handleUpgrade('yearly')}
                                >
                                    {subscription?.plan === 'yearly' ? 'Active' : loading ? 'Processing...' : 'Subscribe'}
                                </button>
                            </div>
                        </div>
                    </div>
                )}

                {view === 'account' && (
                    <div>
                        <h2 style={{ marginBottom: '2rem' }}>👤 Account</h2>

                        <div className="account-card">
                            <h3>Subscription</h3>
                            <div className="account-row">
                                <span>Plan</span>
                                <span style={{ color: isPremium ? '#00d7ff' : '#888', fontWeight: 'bold' }}>
                                    {String(subscription?.plan || 'free').toUpperCase()}
                                </span>
                            </div>
                            <div className="account-row">
                                <span>Status</span>
                                <span className={`status-badge ${subscription?.status}`}>
                                    {subscription?.status?.toUpperCase()}
                                </span>
                            </div>
                            {isPremium && (
                                <>
                                    <div className="account-row">
                                        <span>Next billing date</span>
                                        <span>{subscription?.expiryDate ? new Date(subscription.expiryDate).toLocaleDateString() : '—'}</span>
                                    </div>
                                    <div className="account-row">
                                        <span>Price</span>
                                        <span>${subscription?.price?.toFixed(2)}</span>
                                    </div>
                                    <div className="account-row">
                                        <span>Auto-Renewal</span>
                                        <label className="toggle">
                                            <input type="checkbox" checked={subscription?.autoRenew || false} onChange={handleToggleAutoRenew} />
                                            <span className="slider"></span>
                                        </label>
                                    </div>
                                </>
                            )}
                        </div>

                        {payments.length > 0 && (
                            <div className="account-card" style={{ marginTop: '1.5rem' }}>
                                <h3>Payment History</h3>
                                <table className="payment-table">
                                    <thead>
                                        <tr><th>Date</th><th>Plan</th><th>Amount</th><th>Status</th></tr>
                                    </thead>
                                    <tbody>
                                        {payments.map(p => (
                                            <tr key={p.id}>
                                                <td>{new Date(p.createdAt).toLocaleDateString()}</td>
                                                <td>{p.plan}</td>
                                                <td>${p.amount.toFixed(2)}</td>
                                                <td><span className={`status-badge ${p.status}`}>{p.status}</span></td>
                                            </tr>
                                        ))}
                                    </tbody>
                                </table>
                            </div>
                        )}
                    </div>
                )}
            </main>
        </div>
    );
}

export default App;
