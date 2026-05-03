document.addEventListener('DOMContentLoaded', () => {
    lucide.createIcons();

    const API_HOST = window.location.port === '8080' ? '' : 'http://localhost:8080';
    const API_BASE = `${API_HOST}/api/v1`;
    const token = localStorage.getItem('certosc_token');
    const username = localStorage.getItem('certosc_username');
    
    if (!token) {
        window.location.href = 'login.html';
        return;
    }

    document.getElementById('username-display').textContent = username;

    document.getElementById('btn-logout').addEventListener('click', () => {
        localStorage.clear();
        window.location.href = 'login.html';
    });

    // Navigation
    const navLinks = document.querySelectorAll('.nav-links a');
    const pages = document.querySelectorAll('.page');
    navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();
            navLinks.forEach(l => l.classList.remove('active'));
            link.classList.add('active');
            const targetId = 'page-' + link.dataset.page;
            pages.forEach(p => p.classList.remove('active'));
            document.getElementById(targetId).classList.add('active');
            document.getElementById('page-title').textContent = link.textContent.trim();
        });
    });

    // Sliders
    document.getElementById('job-cpu').addEventListener('input', e => document.getElementById('val-cpu').textContent = e.target.value);
    document.getElementById('job-ram').addEventListener('input', e => document.getElementById('val-ram').textContent = e.target.value);

    async function apiCall(endpoint, method = 'GET', body = null) {
        try {
            const res = await fetch(`${API_BASE}${endpoint}`, {
                method,
                headers: { 'Content-Type': 'application/json', 'Authorization': `Bearer ${token}` },
                body: body ? JSON.stringify(body) : null
            });
            
            const resText = await res.text();
            if (res.status === 401 || res.status === 403) {
                localStorage.clear();
                window.location.href = 'login.html';
                return { success: false };
            }

            try {
                return JSON.parse(resText);
            } catch (e) {
                console.error(`JSON Parse Error for ${endpoint}:`, resText);
                return { success: false, error: 'Invalid server response' };
            }
        } catch (e) {
            console.error('Network/API Error:', e);
            return { success: false, error: e.message };
        }
    }

    async function loadMyJobs() {
        const res = await apiCall('/jobs');
        if (res.success) {
            const tbody = document.querySelector('#user-jobs-table tbody');
            tbody.innerHTML = '';
            
            // Only show user's own jobs
            const myJobs = res.jobs.filter(j => j.username === username).sort((a,b) => b.submitted_at - a.submitted_at);
            
            myJobs.forEach(job => {
                let statusClass = 'status-pending';
                let statusText = 'Queued';
                if (job.status === 1) { statusClass = 'status-running'; statusText = 'Assigned'; }
                else if (job.status === 2) { statusClass = 'status-running'; statusText = 'Running'; }
                else if (job.status === 3) { statusClass = 'status-success'; statusText = 'Completed'; }
                else if (job.status === 4) { statusClass = 'status-failed'; statusText = 'Failed'; }
                else if (job.status === 5) { statusClass = 'status-failed'; statusText = 'Cancelled'; }

                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td><code>${job.job_id.substr(0,8)}</code></td>
                    <td>${job.spec.name}</td>
                    <td><span class="status-badge ${statusClass}">${statusText}</span></td>
                    <td>${job.assigned_node_id || '-'}</td>
                    <td>
                        ${(job.status < 3) ? `<button class="btn btn-danger" style="padding: 4px 8px; font-size: 0.8em; background: #e74c3c; color: white; border: none; border-radius: 4px;" onclick="cancelJob('${job.job_id}')">Cancel</button>` : ''}
                    </td>
                `;
                tbody.appendChild(tr);
            });
        }
    }

    window.cancelJob = async (jobId) => {
        if (!confirm(`Are you sure you want to cancel job ${jobId}?`)) return;
        const res = await apiCall(`/jobs/${jobId}`, 'DELETE');
        if (res.success) {
            loadMyJobs();
        } else {
            alert('Failed to cancel job: ' + res.error);
        }
    };

    document.getElementById('btn-submit-job').addEventListener('click', async () => {
        const name = document.getElementById('job-name').value;
        const cmd = document.getElementById('job-cmd').value;
        if (!name || !cmd) return alert('Name and command required');

        const req = { name, command: cmd };
        const res = await apiCall('/jobs', 'POST', req);
        
        if (res.success) {
            document.getElementById('job-name').value = '';
            document.getElementById('job-cmd').value = '';
            document.querySelector('[data-page=jobs]').click();
            loadMyJobs();
        } else {
            alert('Submission failed: ' + res.error);
        }
    });

    loadMyJobs();
    setInterval(loadMyJobs, 2000);
});
