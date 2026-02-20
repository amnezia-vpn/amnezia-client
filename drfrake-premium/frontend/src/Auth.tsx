// @ts-nocheck
import { useState } from 'react';
import { Login, Register } from "../wailsjs/go/main/App";

interface AuthProps {
    onLogin: (user: any) => void;
}

export function Auth({ onLogin }: AuthProps) {
    const [isLogin, setIsLogin] = useState(true);
    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [error, setError] = useState('');
    const [loading, setLoading] = useState(false);

    const handleSubmit = async (e: React.FormEvent) => {
        e.preventDefault();
        setError('');
        setLoading(true);

        try {
            let user;
            if (isLogin) {
                user = await Login(email, password);
            } else {
                user = await Register(email, password);
            }
            onLogin(user);
        } catch (err: any) {
            setError(err || "Authentication failed");
        }
        setLoading(false);
    };

    return (
        <div style={{
            display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center',
            height: '100vh', width: '100%', background: 'linear-gradient(135deg, #050a14, #0a1525)'
        }}>
            <div style={{
                background: 'rgba(255, 255, 255, 0.05)', backdropFilter: 'blur(10px)',
                padding: '3rem', borderRadius: '24px', border: '1px solid rgba(255,255,255,0.1)',
                width: '320px', textAlign: 'center'
            }}>
                <h1 style={{ color: '#00d7ff', marginBottom: '2rem', letterSpacing: '2px' }}>DR. FRAKE</h1>

                <form onSubmit={handleSubmit} style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
                    <input
                        type="email"
                        placeholder="Email"
                        value={email}
                        onChange={e => setEmail(e.target.value)}
                        required
                        style={{
                            padding: '12px', borderRadius: '8px', border: '1px solid rgba(255,255,255,0.2)',
                            background: 'rgba(0,0,0,0.3)', color: 'white', outline: 'none'
                        }}
                    />
                    <input
                        type="password"
                        placeholder="Password"
                        value={password}
                        onChange={e => setPassword(e.target.value)}
                        required
                        style={{
                            padding: '12px', borderRadius: '8px', border: '1px solid rgba(255,255,255,0.2)',
                            background: 'rgba(0,0,0,0.3)', color: 'white', outline: 'none'
                        }}
                    />

                    {error && <div style={{ color: '#ff4d4d', fontSize: '0.85rem', margin: '0.5rem 0' }}>{error}</div>}

                    <button type="submit" className="btn-primary" disabled={loading}>
                        {loading ? 'Processing...' : isLogin ? 'Sign In' : 'Create Account'}
                    </button>
                </form>

                <div style={{ marginTop: '1.5rem', fontSize: '0.9rem', color: '#aaa' }}>
                    {isLogin ? "Don't have an account? " : "Already have an account? "}
                    <span
                        onClick={() => { setIsLogin(!isLogin); setError(''); }}
                        style={{ color: '#00d7ff', cursor: 'pointer', fontWeight: 'bold' }}
                    >
                        {isLogin ? 'Sign Up' : 'Log In'}
                    </span>
                </div>
            </div>
        </div>
    );
}
