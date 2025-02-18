import subprocess

from models import db, Resources


class utilities:
    
    def __init__(self):
        if self.check_dig_installed:
            pass
    
    def run_dig(domain, query_type="A", dns_server="8.8.8.8"):
        """Runs a dig query and stores the result in the database."""
        command = ["dig", f"@{dns_server}", domain, query_type, "+noall", "+answer", "+stats"]
        result = subprocess.run(command, capture_output=True, text=True)
        
        output_lines = result.stdout.split("\n")
        
        # Extracting relevant sections
        answer_section = []
        query_time = 0.0

        for line in output_lines:
            if line.strip().startswith(domain):
                answer_section.append(line.strip())
            elif "Query time:" in line:
                query_time = float(line.split(":")[1].strip().split()[0])

        answer_text = "\n".join(answer_section) if answer_section else "No records found"

        # Store in database
        dig_result = Resources(
            domain=domain,
            query_type=query_type,
            answer_section=answer_text,
            query_time=query_time,
            dns_server=dns_server
        )
        db.session.add(dig_result)
        db.session.commit()


    def check_dig_installed():
        try:
            # Run the 'dig' command with a simple query (e.g., a lookup for a domain)
            result = subprocess.run(['dig', '@8.8.8.8', 'example.com'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            
            # Check if dig executed successfully
            if result.returncode == 0:
                print("dig is installed.")
                return True
            else:
                print("dig is not installed.")
                return False
        except FileNotFoundError:
            # This exception is raised if the command is not found
            print("dig is not installed.")
            return False