import subprocess

def run_remote():
    # 1. Restore trauth.php from /usr/local/bin
    # 2. Restore trauth_wrapper.sh from base64
    # 3. Fix permissions for .erlang.cookie (400) and others
    # 4. Restart ejabberd
    
    wrapper_b64 = "IyEvYmluL3NoCi91c3IvYmluL3BocCAvdmFyL2xpYi9lamFiYmVyZC90cmF1dGgucGhwCg=="
    
    cmds = [
        "cp /usr/local/bin/trauth.php /var/lib/ejabberd/trauth.php",
        f"echo {wrapper_b64} | base64 -d > /var/lib/ejabberd/trauth_wrapper.sh",
        "chown -R ejabberd:ejabberd /var/lib/ejabberd",
        "chmod 755 /var/lib/ejabberd/trauth.php /var/lib/ejabberd/trauth_wrapper.sh",
        "chmod 400 /var/lib/ejabberd/.erlang.cookie || true",
        "systemctl restart ejabberd"
    ]
    
    remote_cmd = 'echo esg!@#$ | sudo -S sh -c "' + ' && '.join(cmds) + '"'
    
    # The full plink command
    full_cmd = ['plink', '-pw', 'esg!@#$', 'esg@59.187.96.23', remote_cmd]
    
    print(f"Running: {' '.join(full_cmd)}")
    result = subprocess.run(full_cmd, capture_output=True, text=True)
    print("STDOUT:", result.stdout)
    print("STDERR:", result.stderr)

if __name__ == "__main__":
    run_remote()
