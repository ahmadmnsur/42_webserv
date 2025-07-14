// Mobile Navigation Toggle
document.addEventListener('DOMContentLoaded', function() {
    const hamburger = document.querySelector('.hamburger');
    const navMenu = document.querySelector('.nav-menu');
    
    hamburger.addEventListener('click', function() {
        navMenu.classList.toggle('active');
    });
    
    // Close menu when clicking on a link
    document.querySelectorAll('.nav-link').forEach(link => {
        link.addEventListener('click', function() {
            navMenu.classList.remove('active');
        });
    });
});

// Smooth scrolling for navigation links
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
        e.preventDefault();
        const target = document.querySelector(this.getAttribute('href'));
        if (target) {
            target.scrollIntoView({
                behavior: 'smooth',
                block: 'start'
            });
        }
    });
});

// Test functions for the interactive demo
async function testMethod(method) {
    const resultDiv = document.getElementById('method-result');
    resultDiv.innerHTML = `<div class="loading">Testing ${method}...</div>`;
    
    try {
        const response = await fetch('/', {
            method: method,
            headers: {
                'Content-Type': 'text/plain'
            }
        });
        
        const statusClass = response.ok ? 'success' : 'error';
        resultDiv.innerHTML = `
            <div class="${statusClass}">
                <strong>${method}</strong>: ${response.status} ${response.statusText}
                <br>Headers: ${JSON.stringify(Object.fromEntries(response.headers.entries()), null, 2)}
            </div>
        `;
    } catch (error) {
        resultDiv.innerHTML = `<div class="error">Error: ${error.message}</div>`;
    }
}

async function testConcurrency() {
    const resultDiv = document.getElementById('perf-result');
    resultDiv.innerHTML = '<div class="loading">Testing concurrent requests...</div>';
    
    const startTime = Date.now();
    const requests = [];
    
    // Create 10 concurrent requests
    for (let i = 0; i < 10; i++) {
        requests.push(fetch(`/?test=${i}`));
    }
    
    try {
        const responses = await Promise.all(requests);
        const endTime = Date.now();
        const duration = endTime - startTime;
        
        const successCount = responses.filter(r => r.ok).length;
        
        resultDiv.innerHTML = `
            <div class="success">
                <strong>Concurrent Test Results:</strong>
                <br>Requests: 10
                <br>Successful: ${successCount}
                <br>Duration: ${duration}ms
                <br>Avg: ${(duration/10).toFixed(2)}ms per request
            </div>
        `;
    } catch (error) {
        resultDiv.innerHTML = `<div class="error">Error: ${error.message}</div>`;
    }
}

async function testLargeFile() {
    const resultDiv = document.getElementById('perf-result');
    resultDiv.innerHTML = '<div class="loading">Testing large file...</div>';
    
    // Create a large payload
    const largeData = 'A'.repeat(10000);
    
    try {
        const startTime = Date.now();
        const response = await fetch('/', {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain'
            },
            body: largeData
        });
        const endTime = Date.now();
        
        const statusClass = response.ok ? 'success' : 'error';
        resultDiv.innerHTML = `
            <div class="${statusClass}">
                <strong>Large File Test:</strong>
                <br>Data Size: ${largeData.length} bytes
                <br>Status: ${response.status} ${response.statusText}
                <br>Duration: ${endTime - startTime}ms
            </div>
        `;
    } catch (error) {
        resultDiv.innerHTML = `<div class="error">Error: ${error.message}</div>`;
    }
}

async function testSecurity() {
    const resultDiv = document.getElementById('security-result');
    resultDiv.innerHTML = '<div class="loading">Testing path traversal...</div>';
    
    const maliciousPath = '../../../etc/passwd';
    
    try {
        const response = await fetch(`/${maliciousPath}`);
        const statusClass = response.status === 404 ? 'success' : 'error';
        const message = response.status === 404 ? 'Path traversal blocked ✓' : 'Path traversal vulnerability!';
        
        resultDiv.innerHTML = `
            <div class="${statusClass}">
                <strong>Path Traversal Test:</strong>
                <br>Path: ${maliciousPath}
                <br>Status: ${response.status} ${response.statusText}
                <br>Result: ${message}
            </div>
        `;
    } catch (error) {
        resultDiv.innerHTML = `<div class="error">Error: ${error.message}</div>`;
    }
}

async function testHeaders() {
    const resultDiv = document.getElementById('security-result');
    resultDiv.innerHTML = '<div class="loading">Testing header validation...</div>';
    
    try {
        const response = await fetch('/', {
            headers: {
                'X-Test': 'Normal-Header',
                'X-Custom': 'Safe-Value-123'
            }
        });
        
        const statusClass = response.ok ? 'success' : 'error';
        resultDiv.innerHTML = `
            <div class="${statusClass}">
                <strong>Header Validation Test:</strong>
                <br>Status: ${response.status} ${response.statusText}
                <br>Result: Headers processed safely ✓
                <br>Note: Browser blocks malicious header injection
            </div>
        `;
    } catch (error) {
        resultDiv.innerHTML = `<div class="error">Error: ${error.message}</div>`;
    }
}

// Add loading and result styles
const style = document.createElement('style');
style.textContent = `
    .loading {
        color: #f59e0b;
        font-weight: 500;
    }
    
    .success {
        color: #10b981;
        font-weight: 500;
    }
    
    .error {
        color: #ef4444;
        font-weight: 500;
    }
    
    .test-result {
        white-space: pre-wrap;
        word-wrap: break-word;
    }
`;
document.head.appendChild(style);

// Add intersection observer for animations
const observerOptions = {
    threshold: 0.1,
    rootMargin: '0px 0px -50px 0px'
};

const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
        if (entry.isIntersecting) {
            entry.target.style.opacity = '1';
            entry.target.style.transform = 'translateY(0)';
        }
    });
}, observerOptions);

// Observe all cards for animation
document.querySelectorAll('.feature-card, .demo-card, .cgi-card, .test-card').forEach(card => {
    card.style.opacity = '0';
    card.style.transform = 'translateY(20px)';
    card.style.transition = 'opacity 0.6s ease, transform 0.6s ease';
    observer.observe(card);
});

// Add navbar scroll effect
window.addEventListener('scroll', () => {
    const navbar = document.querySelector('.navbar');
    if (window.scrollY > 100) {
        navbar.style.background = 'rgba(255, 255, 255, 0.98)';
        navbar.style.boxShadow = '0 2px 20px rgba(0, 0, 0, 0.1)';
    } else {
        navbar.style.background = 'rgba(255, 255, 255, 0.95)';
        navbar.style.boxShadow = 'none';
    }
});