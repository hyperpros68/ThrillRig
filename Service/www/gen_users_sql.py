import hashlib

def get_hash(pw, user_id):
    return hashlib.sha256((pw + user_id).encode()).hexdigest()

accounts = [
    # [login_id, password, role, name]
    ("trllm", "trllm!@#$", "ADMIN", "LLM Server"),
    ("trwww", "trwww!@#$", "ADMIN", "Service Server"),
    ("adp_shopid_001", "tradp!@#$", "ADAPTER", "Adapter Shop 001"),
    ("shop_001", "trshop!@#$", "SHOP", "Shop 001"),
    ("agt_shopid_001", "tragt!@#$", "AGENT", "Agent Shop 001"),
    ("mgr_shopid_001", "trmgr!@#$", "MANAGER", "Manager App 001"),
    
    # Test accounts
    ("test_tradp_001_001", "tradp!@#$", "ADAPTER", "Test Adapter 001"),
    ("test_tradp_001_002", "tradp!@#$", "ADAPTER", "Test Adapter 002"),
    ("test_tradp_001_003", "tradp!@#$", "ADAPTER", "Test Adapter 003"),
    
    ("test_tragt_001_001", "tragt!@#$", "AGENT", "Test Agent 001"),
    ("test_tragt_001_002", "tragt!@#$", "AGENT", "Test Agent 002"),
    ("test_tragt_001_003", "tragt!@#$", "AGENT", "Test Agent 003"),
    
    ("test_shop_001", "trshop!@#$", "SHOP", "Test Shop 001"),
    ("test_shop_002", "trshop!@#$", "SHOP", "Test Shop 002"),
    ("test_shop_003", "trshop!@#$", "SHOP", "Test Shop 003"),
    
    ("test_trmgr_001", "trmgr!@#$", "MANAGER", "Test Manager 001"),
    ("test_trmgr_002", "trmgr!@#$", "MANAGER", "Test Manager 002"),
    ("test_trmgr_003", "trmgr!@#$", "MANAGER", "Test Manager 003"),
    
    ("test_trllm", "trllm!@#$", "ADMIN", "Test LLM"),
    ("test_trwww", "trwww!@#$", "ADMIN", "Test WWW"),
    
    # Admin
    ("admin", "esg!@#$", "ADMIN", "Administrator")
]

sql = "REPLACE INTO tr_users (login_id, role, password, name) VALUES "
values = []
for uid, pw, role, name in accounts:
    h = get_hash(pw, uid)
    values.append(f"('{uid}', '{role}', '{h}', '{name}')")

sql += ", ".join(values) + ";"

# Execute on server
import subprocess
cmd = f'plink -pw esg!@#$ esg@59.187.96.23 "mysql -uthrillrig -ptr!@#$ ThrillRig -e \\\"{sql}\\\""'
print("Updating Database...")
result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
if result.returncode == 0:
    print("Database updated successfully!")
else:
    print("Error updating database:")
    print(result.stderr)
