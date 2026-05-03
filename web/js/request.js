document.addEventListener('DOMContentLoaded', () => {
    const form = document.getElementById('request-form');
    const fileInput = document.getElementById('file-input');
    const fileNameDisplay = document.getElementById('file-name');
    const errorMsg = document.getElementById('error-msg');
    const submitBtn = document.getElementById('submit-btn');

    fileInput.addEventListener('change', (e) => {
        if (e.target.files.length > 0) {
            fileNameDisplay.textContent = e.target.files[0].name;
        }
    });

    form.addEventListener('submit', async (e) => {
        e.preventDefault();
        submitBtn.disabled = true;
        submitBtn.textContent = 'Submitting...';
        errorMsg.style.display = 'none';

        const API_HOST = window.location.port === '8080' ? '' : 'http://localhost:8080';

        const metadata = {
            name: document.getElementById('name').value,
            email: document.getElementById('email').value,
            container: document.getElementById('req-container').value,
            goal: document.getElementById('goal').value,
            cpu: parseInt(document.getElementById('cpu').value),
            ram: parseInt(document.getElementById('ram').value),
            time: parseInt(document.getElementById('time').value)
        };

        try {
            // Step 1: Submit Metadata
            const res = await fetch(`${API_HOST}/api/v1/public/request`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(metadata)
            });
            const resText = await res.text();
            let data;
            try {
                data = JSON.parse(resText);
            } catch (e) {
                console.error("Failed to parse JSON. Response text:", resText);
                throw new Error("Invalid server response");
            }

            if (!data.success) throw new Error(data.error || 'Metadata submission failed');

            const requestId = data.request_id;
            
            // Step 2: Upload File
            const file = fileInput.files[0];
            submitBtn.textContent = 'Uploading Code...';
            
            const uploadRes = await fetch(`${API_HOST}/api/v1/public/request/${requestId}/upload`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/octet-stream' },
                body: file
            });
            const uploadText = await uploadRes.text();
            let uploadData;
            try {
                uploadData = JSON.parse(uploadText);
            } catch (e) {
                console.error("Failed to parse Upload JSON. Response text:", uploadText);
                throw new Error("Invalid upload response");
            }

            if (!uploadData.success) throw new Error(uploadData.error || 'File upload failed');

            // Success!
            document.getElementById('request-form-container').style.display = 'none';
            document.getElementById('success-container').style.display = 'block';
            document.getElementById('res-id').textContent = requestId;

        } catch (err) {
            errorMsg.textContent = err.message;
            errorMsg.style.display = 'block';
            submitBtn.disabled = false;
            submitBtn.textContent = 'Submit Request';
        }
    });
});
