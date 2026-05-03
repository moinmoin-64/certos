document.addEventListener('DOMContentLoaded', () => {
    lucide.createIcons();

    const form = document.getElementById('login-form');
    const errorMsg = document.getElementById('error-msg');
    const API_HOST = window.location.port === '8080' ? '' : 'http://localhost:8080';
    const API_BASE = `${API_HOST}/api/v1`;

    form.addEventListener('submit', async (e) => {
        e.preventDefault();
        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;

        try {
            const res = await fetch(`${API_BASE}/auth/login`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, password })
            });
            const data = await res.json();
            
            if (data.success) {
                localStorage.setItem('certosc_token', data.access_token);
                localStorage.setItem('certosc_username', data.user.username);
                localStorage.setItem('certosc_role', data.user.role);
                
                if (data.user.role === 'admin') {
                    window.location.href = 'admin.html';
                } else {
                    window.location.href = 'user.html';
                }
            } else {
                errorMsg.textContent = data.error || 'Login failed';
                errorMsg.style.display = 'block';
            }
        } catch (err) {
            errorMsg.textContent = 'Failed to connect to Gateway';
            errorMsg.style.display = 'block';
        }
    });
});
