document.addEventListener('DOMContentLoaded', () => {
    lucide.createIcons();

    const API_HOST = window.location.port === '8080' ? '' : 'http://localhost:8080';
    const API_BASE = `${API_HOST}/api/v1`;
    const token = localStorage.getItem('certosc_token');
    const username = localStorage.getItem('certosc_username');
    const role = localStorage.getItem('certosc_role');
    
    // DOM Elements
    const requestsList = document.getElementById('requests-list');
    const jobsList = document.getElementById('jobs-list');
    const nodesList = document.getElementById('nodes-list');
    const userList = document.getElementById('user-list');

    if (!token || role !== 'admin') {
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

    async function refreshDashboard() {
        // 1. Cluster Status
        const clusterRes = await apiCall('/cluster');
        if (clusterRes.success) {
            document.getElementById('stat-running').textContent = clusterRes.running_jobs;
            document.getElementById('stat-queued').textContent = clusterRes.queued_jobs;
        }

        // 2. Nodes
        const nodesRes = await apiCall('/nodes');
        if (nodesRes.success) {
            const onlineNodes = nodesRes.nodes.filter(n => n.status === 'online');
            document.getElementById('stat-nodes').textContent = `${onlineNodes.length} / ${nodesRes.nodes.length}`;
            
            const heatmapGrid = document.getElementById('node-heatmap');
            const nodesGrid = document.getElementById('nodes-detailed-grid');
            heatmapGrid.innerHTML = '';
            nodesGrid.innerHTML = '';

            nodesRes.nodes.forEach(node => {
                const cell = document.createElement('div');
                cell.className = `node-cell ${node.status === 'online' ? 'node-idle' : 'node-dead'}`;
                cell.title = `${node.hostname} (${node.ip})`;
                cell.textContent = node.hostname.split('-').pop() || node.hostname;
                heatmapGrid.appendChild(cell);

                const card = document.createElement('div');
                card.className = 'panel glass';
                card.style.margin = '0';
                card.innerHTML = `
                    <div style="display: flex; justify-content: space-between;">
                        <h3>${node.hostname}</h3>
                        <span class="status-badge ${node.status === 'online' ? 'status-completed' : 'status-failed'}">${node.status}</span>
                    </div>
                    <div style="margin-top: 10px; font-size: 0.9em; color: var(--text-muted);">
                        <p>IP: ${node.ip}</p>
                        <p>CPU Cores: ${node.resources.cpu_cores}</p>
                        <p>RAM: ${node.resources.ram_mb} MB</p>
                    </div>
                `;
                nodesGrid.appendChild(card);
            });
        }

        // 3. Pending Requests
        const requestsRes = await apiCall('/admin/requests');
        if (requestsRes.success) {
            const tableBody = document.querySelector('#requests-table tbody');
            tableBody.innerHTML = '';
            
            requestsRes.requests.filter(r => r.status === 0).forEach(req => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td><code>${req.request_id.substr(0,8)}</code></td>
                    <td><strong>${req.applicant_name}</strong><br><small>${req.applicant_email}</small></td>
                    <td><div style="max-width: 250px; font-size: 0.9em;">${req.project_goal}</div></td>
                    <td><small>${req.requested_spec.resources.cpu_cores} Cores, ${req.requested_spec.resources.ram_mb/1024}GB RAM</small></td>
                    <td><span class="status-badge status-pending">Pending</span></td>
                    <td>
                        <button class="btn btn-sm btn-success btn-approve" data-id="${req.request_id}">Approve</button>
                        <button class="btn btn-sm btn-danger btn-reject" data-id="${req.request_id}">Reject</button>
                    </td>
                `;
                tableBody.appendChild(tr);
            });
        }

        // 4. Jobs
        const jobsRes = await apiCall('/jobs');
        if (jobsRes.success) {
            const allJobsTableBody = document.querySelector('#all-jobs-table tbody');
            allJobsTableBody.innerHTML = '';

            const sortedJobs = jobsRes.jobs.sort((a,b) => b.submitted_at - a.submitted_at);
            
            sortedJobs.forEach(job => {
                let statusClass = 'status-pending';
                let statusText = 'Queued';
                if (job.status === 1) { statusClass = 'status-running'; statusText = 'Assigned'; }
                else if (job.status === 2) { statusClass = 'status-running'; statusText = 'Running'; }
                else if (job.status === 3) { statusClass = 'status-success'; statusText = 'Completed'; }
                else if (job.status === 4) { statusClass = 'status-failed'; statusText = 'Failed'; }
                else if (job.status === 5) { statusClass = 'status-failed'; statusText = 'Cancelled'; }

                const submittedDate = new Date(job.submitted_at * 1000).toLocaleString();
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td><code>${job.job_id.substr(0,8)}</code></td>
                    <td>${job.spec.name}</td>
                    <td>${job.username}</td>
                    <td><span class="status-badge ${statusClass}">${statusText}</span></td>
                    <td>${job.assigned_node_id || '-'}</td>
                    <td>${submittedDate}</td>
                    <td>
                        <button class="btn btn-secondary" style="padding: 4px 8px; font-size: 0.8em;">Details</button>
                        ${(job.status < 3) ? `<button class="btn btn-danger" style="padding: 4px 8px; font-size: 0.8em; margin-left: 5px; background: #e74c3c; color: white; border: none; border-radius: 4px;" onclick="cancelJob('${job.job_id}')">Cancel</button>` : ''}
                    </td>
                `;
                allJobsTableBody.appendChild(tr);
            });
        }
        // 5. Users
        const usersRes = await apiCall('/admin/users');
        if (usersRes.success) {
            const tableBody = document.querySelector('#users-table tbody');
            tableBody.innerHTML = '';
            usersRes.users.forEach(u => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td>${u.username}</td>
                    <td><span class="status-badge ${u.role === 'admin' ? 'status-running' : 'status-queued'}">${u.role}</span></td>
                    <td>
                        <button class="btn btn-secondary" style="padding: 4px 8px; font-size: 0.8em;" onclick="alert('Edit not implemented')">Edit</button>
                        ${u.username !== 'admin' ? `<button class="btn btn-danger" style="padding: 4px 8px; font-size: 0.8em; margin-left: 5px; background: #ef4444; color: white; border: none; border-radius: 4px;" onclick="deleteUser('${u.username}')">Delete</button>` : ''}
                    </td>
                `;
                tableBody.appendChild(tr);
            });
        }
    }

    // Modal Controls
    const modalAddUser = document.getElementById('modal-add-user');
    const openModalBtn = document.getElementById('btn-open-add-user');
    const closeModalBtn = document.getElementById('btn-close-add-user');

    if (openModalBtn) {
        openModalBtn.addEventListener('click', () => {
            modalAddUser.style.display = 'flex';
        });
    }

    if (closeModalBtn) {
        closeModalBtn.addEventListener('click', () => {
            modalAddUser.style.display = 'none';
        });
    }

    function closeAddUserModal() {
        modalAddUser.style.display = 'none';
    }

    document.getElementById('add-user-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const username = document.getElementById('new-username').value;
        const password = document.getElementById('new-password').value;
        const role = document.getElementById('new-role').value;

        const res = await apiCall('/admin/users', 'POST', { username, password, role });
        if (res.success) {
            closeAddUserModal();
            refreshDashboard();
            e.target.reset();
        } else {
            alert('Failed to create user: ' + res.error);
        }
    });

    window.deleteUser = async (uname) => {
        if (!confirm(`Delete user ${uname}?`)) return;
        const res = await apiCall(`/admin/users/${uname}`, 'DELETE');
        if (res.success) {
            refreshDashboard();
        } else {
            alert('Failed to delete user: ' + res.error);
        }
    };

    async function loadRequests() {
        const data = await apiCall('/admin/requests');
        if (data.requests) {
            requestsList.innerHTML = data.requests.map(req => `
                <tr class="request-row">
                    <td><span class="badge badge-secondary">${req.request_id}</span></td>
                    <td>${req.applicant_name}<br><small>${req.applicant_email}</small></td>
                    <td>${req.project_goal}</td>
                    <td>
                        <small>CPU: ${req.requested_spec.resources.cpu_cores} | RAM: ${Math.round(req.requested_spec.resources.ram_mb/1024)}GB</small>
                    </td>
                    <td>
                        <span class="status-badge status-${req.status === 0 ? 'pending' : (req.status === 1 ? 'running' : 'failed')}">
                            ${req.status === 0 ? 'PENDING' : (req.status === 1 ? 'APPROVED' : 'REJECTED')}
                        </span>
                    </td>
                    <td>
                        ${req.status === 0 ? `
                            <button class="btn btn-sm btn-success btn-approve" data-id="${req.request_id}">Approve</button>
                            <button class="btn btn-sm btn-danger btn-reject" data-id="${req.request_id}">Reject</button>
                        ` : '<span class="text-muted">Reviewed</span>'}
                    </td>
                </tr>
            `).join('');
        }
    }

    // Event Delegation for Table Actions
    requestsList.addEventListener('click', async (e) => {
        const btn = e.target;
        if (!btn.classList.contains('btn-approve') && !btn.classList.contains('btn-reject')) return;

        const id = btn.dataset.id;
        const isApprove = btn.classList.contains('btn-approve');
        
        btn.disabled = true;
        btn.textContent = 'Processing...';

        const res = await apiCall(`/admin/requests/${id}/review`, 'POST', {
            approve: isApprove,
            comment: isApprove ? 'Approved by admin' : 'Rejected by admin'
        });

        if (res.success) {
            loadRequests();
            loadJobs();
        } else {
            alert(`Failed to ${isApprove ? 'approve' : 'reject'} request: ` + (res.error || 'Unknown error'));
            btn.disabled = false;
            btn.textContent = isApprove ? 'Approve' : 'Reject';
        }
    });

    window.cancelJob = async (jobId) => {
        if (!confirm(`Are you sure you want to cancel job ${jobId}?`)) return;
        const res = await apiCall(`/jobs/${jobId}`, 'DELETE');
        if (res.success) {
            refreshDashboard();
        } else {
            alert('Failed to cancel job: ' + res.error);
        }
    };

    refreshDashboard();
    setInterval(refreshDashboard, 2000);
});
