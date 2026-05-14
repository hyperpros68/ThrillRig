# 베이스 이미지로 아나콘다(Miniconda) 사용
FROM continuumio/miniconda3:latest

# 작업 디렉토리 설정
WORKDIR /app

# 환경 설정 파일 복사 및 환경 생성
COPY environment.yml .
RUN conda env create -f environment.yml

# 콘다 환경을 기본 쉘로 설정
SHELL ["conda", "run", "-n", "trbrain_env", "/bin/bash", "-c"]

# 소스 코드 복사 (빌드 시점)
COPY . .

# 포트 개방 (TRBrain: 19990)
EXPOSE 19990

# 실행 명령어
ENTRYPOINT ["conda", "run", "--no-capture-output", "-n", "trbrain_env", "python", "main/main.py"]
